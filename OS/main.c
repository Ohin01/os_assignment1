#define _DEFAULT_SOURCE   // enables extra POSIX features

#include <stdio.h>   
#include <stdlib.h>      
#include <string.h>      
#include <unistd.h>    
#include <sys/wait.h>     
#include <sys/types.h>    
#include <sys/stat.h>     
#include <fcntl.h>        
#include <dirent.h>       
#include <pwd.h>          
#include <sys/utsname.h>  
#include <time.h>         
#include <errno.h>       
#include <limits.h>       

#define MAX_SIZE     1024
#define MAX_ARG      64   
#define BUF_SIZE     4096  
#define HISTORY_MAX  50   

char *argv[MAX_ARG];      
int   argc = 0;           
char  command[MAX_SIZE];   

char rootDir[PATH_MAX];   
char cwd[PATH_MAX];       

char history[HISTORY_MAX][MAX_SIZE]; 
int  history_count = 0;            

void printDir(void)
{
    if (getcwd(cwd, sizeof(cwd)) == NULL) 
    {
        perror("getcwd");              
        printf("myshell> ");           
        return;
    }
    printf("%s: ", cwd);                
}

void addHistory(const char *cmd)
{
    if (cmd[0] == '\0') return;          

    int slot = history_count % HISTORY_MAX; //wraps
    strncpy(history[slot], cmd, MAX_SIZE - 1);
    history[slot][MAX_SIZE - 1] = '\0'; //null at the end

    history_count++;                          
}

void takeCommand(void)
{
    if (fgets(command, sizeof(command), stdin) == NULL) //read line from stdin
    {
        printf("\nExiting shell.\n");
        exit(0);
    }
    command[strcspn(command, "\n")] = '\0'; //replace trailing '\n' with '\0'
    if (!isatty(STDIN_FILENO)) printf("%s\n", command); // echo input piped mode
}

void parseCommand(void)
{
    argc = 0;                                
    argv[argc] = strtok(command, " ");         

    while (argv[argc] != NULL && argc < MAX_ARG - 1)
    {
        argc++;                                
        argv[argc] = strtok(NULL, " "); // get next token
    }
}

int insideRoot(const char *path)
{
    if (path == NULL || path[0] == '\0') return 0;

    char actual[PATH_MAX]; 

    if (realpath(path, actual) != NULL)
    {
        size_t len = strlen(rootDir);
        if (strncmp(actual, rootDir, len) != 0) return 0; // check prefix matches root
        return (actual[len] == '\0' || actual[len] == '/'); // exact or subdirectory match
    }

    // if path does not exist yet
    char parent[PATH_MAX];
    const char *slash = strrchr(path, '/');

    if (slash == NULL) // if no slash in path, parent is current directory
    {
        strcpy(parent, ".");
    }
    else if (slash == path) // if leading slash, parent is root
    {
        strcpy(parent, "/"); 
    }
    else // extract path before last slash
    {
        size_t len = (size_t)(slash - path);
        if (len >= sizeof(parent)) return 0;
        memcpy(parent, path, len);
        parent[len] = '\0'; 
    }

    if (realpath(parent, actual) != NULL) // resolve parent directory
    {
        size_t len = strlen(rootDir); 
        if (strncmp(actual, rootDir, len) != 0) return 0; 
        return (actual[len] == '\0' || actual[len] == '/'); 
    }

    return 0;
}

void cmd_pwd(void)
{
    char buf[PATH_MAX];                        
    if (getcwd(buf, sizeof(buf)) == NULL)     
    {
        perror("pwd: getcwd");
        return;
    }
    printf("%s\n", buf);                     
}

// chdir() interprets "/.." as parent directory
void cmd_cd(void)
{
    char target[PATH_MAX * 2]; // holds where to change

    if (argc < 2) //only cd
    {
        strcpy(target, rootDir);              
    }
    else
    {
        if (argv[1][0] == '/') //absolute path
        {
            snprintf(target, sizeof(target), "%s%s", rootDir, argv[1]); //prepend rootDir
        }
        else  //relative path argument
        {
            getcwd(target, sizeof(target)); 
            strcat(target, "/");
            strcat(target, argv[1]); 
        }
    }

    char resolved[PATH_MAX]; 
    if (realpath(target, resolved) == NULL) //resolve target path
    {
        perror("cd");
        return;
    }

    if (!insideRoot(resolved)) //if outside root
    {
        chdir(rootDir);
        printf("cd: cannot go above the shell root directory\n");
        return;
    }

    if (chdir(resolved) != 0) 
    {
        perror("cd"); 
        return;
    }
}

void cmd_echo(void)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '$') //environment variable reference
        {
            char *val = getenv(argv[i] + 1); //look up variable, skip '$'
            printf("%s", val ? val : "");
        }
        else
        {
            printf("%s", argv[i]);           
        }

        if (i < argc - 1) printf(" ");   
    }
    printf("\n");                           
}

void cmd_history(void)
{
    int total = (history_count < HISTORY_MAX) ? history_count : HISTORY_MAX;

    if (total == 0) {printf("No commands in history.\n"); return;}

    int start    = (history_count > HISTORY_MAX) ? (history_count % HISTORY_MAX) : 0;
    int first_num = history_count - total + 1;

    for (int i = 0; i < total; i++)
    {
        int slot = (start + i) % HISTORY_MAX;  //circular index
        printf("%d  %s\n", first_num + i, history[slot]);
    }
}

void cmd_help(void)
{
    printf("=== Custom Shell — Commands ===\n");
    printf("  pwd                  Print current working directory\n");
    printf("  ls [-l] [-a] [dir]   List directory contents (-l: details, -a: all)\n");
    printf("  cd [dir]             Change directory (cannot go above shell root)\n");
    printf("  whoami               Show current user name\n");
    printf("  uname                Show system name and architecture\n");
    printf("  mkdir <dir>          Create directory\n");
    printf("  touch <file>         Create empty file or update timestamp\n");
    printf("  rm <path>            Remove file or empty directory\n");
    printf("  cp <src> <dst>       Copy file\n");
    printf("  mv <src> <dst>       Move / rename file or directory\n");
    printf("  cat <file>           Display file contents\n");
    printf("  echo [text]          Print text (supports $VAR expansion)\n");
    printf("  uptime               Show system uptime and load averages\n");
    printf("  meminfo              Show memory statistics\n");
    printf("  history              Show command history\n");
    printf("  help                 Show this help message\n");
    printf("  exit [code]          Exit the shell with optional status code\n");
    printf("\n  Piping: cmd1 | cmd2   stdout of cmd1 becomes stdin of cmd2\n");
}

void cmd_ls(void)
{
    int flag_l = 0;                           
    int flag_a = 0;                           
    const char *dir = "."; // target directory path, default is "."

    int dir_found = 0;

    for (int i = 1; i < argc; i++)            
    {
        if (argv[i][0] == '-') 
        {
            for (int j = 1; argv[i][j] != '\0'; j++)
            {
                if (argv[i][j] == 'l') flag_l = 1;    
                else if (argv[i][j] == 'a') flag_a = 1; 
                else
                {
                    fprintf(stderr, "ls: invalid option -- '%c'\n", argv[i][j]);
                    return;                  
                }
            }
        }
        else // argument is a path
        {
            // reject multiple directory arguments
            if (dir_found)
            {
                fprintf(stderr, "ls: too many directory arguments\n");
                return;
            }
            dir = argv[i];
            dir_found = 1; 
        }
    }

    if (!insideRoot(dir))
    {
        fprintf(stderr, "ls: cannot access '%s': outside shell root\n", dir);
        return;
    }

    DIR *dp = opendir(dir); // open the directory stream
    if (dp == NULL)
    {
        fprintf(stderr, "ls: cannot open '%s': %s\n",
                dir, strerror(errno));
        return;
    }

    struct dirent *entry; // pointer to the current directory entry

    while ((entry = readdir(dp)) != NULL)
    {
        if (!flag_a && entry->d_name[0] == '.') // skip hidden entries
            continue;

        if (flag_l)
        {
            char fullpath[PATH_MAX];

            // construct the path and check whether it was truncated
            int len = snprintf(fullpath, sizeof(fullpath), "%s%s%s",
                               dir,
                               (strcmp(dir, "/") == 0 ||
                                dir[strlen(dir) - 1] == '/') ? "" : "/",
                               entry->d_name);

            if (len < 0 || (size_t)len >= sizeof(fullpath))
            {
                fprintf(stderr, "ls: path too long: '%s'\n", entry->d_name);
                continue;
            }

            struct stat st;

            //lstat gets metadata about the link itself
            if (lstat(fullpath, &st) != 0)
            {
                fprintf(stderr, "ls: cannot access '%s': %s\n", fullpath, strerror(errno));
                continue;
            }

            char perms[11];
            perms[0]  = S_ISDIR(st.st_mode) ? 'd' : S_ISLNK(st.st_mode) ? 'l' : '-';
            perms[1]  = (st.st_mode & S_IRUSR) ? 'r' : '-';
            perms[2]  = (st.st_mode & S_IWUSR) ? 'w' : '-';
            perms[3]  = (st.st_mode & S_IXUSR) ? 'x' : '-';
            perms[4]  = (st.st_mode & S_IRGRP) ? 'r' : '-';
            perms[5]  = (st.st_mode & S_IWGRP) ? 'w' : '-';
            perms[6]  = (st.st_mode & S_IXGRP) ? 'x' : '-';
            perms[7]  = (st.st_mode & S_IROTH) ? 'r' : '-';
            perms[8]  = (st.st_mode & S_IWOTH) ? 'w' : '-';
            perms[9]  = (st.st_mode & S_IXOTH) ? 'x' : '-';
            perms[10] = '\0'; 

            char timebuf[32];
            struct tm *tm_info = localtime(&st.st_mtime);

            // check localtime() before passing its result to strftime()
            if (tm_info == NULL || strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info) == 0)
            {
                fprintf(stderr, "ls: cannot format time for '%s'\n", entry->d_name);
                continue;
            }

            printf("%s %8lld %s %s\n", perms, (long long)st.st_size, timebuf, entry->d_name);
        }
        else // short listing format
        {
            printf("%s  ", entry->d_name);
        }
    }

    if (!flag_l) printf("\n");

    closedir(dp); 
}

void cmd_whoami(void)
{
    uid_t uid = getuid(); // get real user ID of process
    struct passwd *pw = getpwuid(uid); // look up passwd entry for UID
    if (pw == NULL) {perror("whoami: getpwuid"); return;}
    printf("%s\n", pw->pw_name);  
}

void cmd_uname(void)
{
    struct utsname uts;
    if (uname(&uts) != 0) {perror("uname"); return;}
    printf("%s %s %s %s\n", uts.sysname, uts.nodename, uts.release, uts.machine);
}

void cmd_mkdir(void)
{
    if (argc < 2) {fprintf(stderr, "mkdir: missing operand\n"); return;}
    if (!insideRoot(argv[1])) {fprintf(stderr, "mkdir: access denied, outside shell root\n"); return;}
    if (mkdir(argv[1], 0755) != 0) perror("mkdir"); // mode 0755 = rwxr-xr-x
}

void cmd_touch(void)
{
    if (argc < 2) {fprintf(stderr, "touch: missing operand\n"); return;}
    if (!insideRoot(argv[1])) {fprintf(stderr, "touch: access denied, outside shell root\n"); return;}

    int fd = open(argv[1], O_CREAT | O_WRONLY, 0644); // create if not exists
    if (fd < 0) { perror("touch: open"); return; }

    close(fd); // close immediately

    if (utimensat(AT_FDCWD, argv[1], NULL, 0) != 0) // NULL = set times to now
        perror("touch: utimensat");
}

void cmd_rm(void)
{
    if (argc < 2) {fprintf(stderr, "rm: missing operand\n"); return;}
    if (!insideRoot(argv[1])) {fprintf(stderr, "rm: access denied, outside shell root\n"); return;}

    struct stat st; // determine file type
    if (stat(argv[1], &st) != 0) {perror("rm: stat"); return;}

    if (S_ISDIR(st.st_mode)) // target is a directory
    {
        if (rmdir(argv[1]) != 0) perror("rm: rmdir");
    }
    else
    {
        if (unlink(argv[1]) != 0) perror("rm: unlink");
    }
}

void cmd_cp(void)
{
    if (argc < 3) {fprintf(stderr, "cp: missing file operand\n"); return;}
    if (!insideRoot(argv[1]) || !insideRoot(argv[2])) {fprintf(stderr, "cp: access denied, outside shell root\n"); return;}

    struct stat st_src, st_dst; // file metadata
    if (stat(argv[1], &st_src) != 0) {perror("cp: source"); return;}
    if (stat(argv[2], &st_dst) == 0 && st_src.st_dev == st_dst.st_dev && st_src.st_ino == st_dst.st_ino)
    {
        fprintf(stderr, "cp: '%s' and '%s' are the same file\n", argv[1], argv[2]); // prevent self-overwrite
        return;
    }

    int src = open(argv[1], O_RDONLY); // open source for reading
    if (src < 0) {perror("cp: open source"); return;}

    int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644); // create/truncate dest
    if (dst < 0) { perror("cp: open destination"); close(src); return; }

    char buf[BUF_SIZE];
    ssize_t n;

    while ((n = read(src, buf, sizeof(buf))) > 0) // read chunk from source
    {
        ssize_t written = 0;
        while (written < n) // write loop, write() may be partial 
        {
            ssize_t w = write(dst, buf + written, n - written);
            if (w < 0) {perror("cp: write"); close(src); close(dst); return;}
            written += w;
        }
    }

    if (n < 0) perror("cp: read");

    close(src);                              
    close(dst);                            
}

void cmd_mv(void)
{
    if (argc < 3) {fprintf(stderr, "mv: missing file operand\n"); return;}
    if (!insideRoot(argv[1]) || !insideRoot(argv[2])) {fprintf(stderr, "mv: access denied, outside shell root\n"); return;}
    if (rename(argv[1], argv[2]) != 0) perror("mv: rename");
}

void cmd_cat(void)
{
    if (argc < 2) {fprintf(stderr, "cat: missing operand\n"); return;}
    if (!insideRoot(argv[1])) {fprintf(stderr, "cat: access denied, outside shell root\n"); return;}

    int fd = open(argv[1], O_RDONLY); // open file for reading 
    if (fd < 0) {perror("cat: open"); return;}

    char buf[BUF_SIZE];
    ssize_t n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) // read until EOF
        write(STDOUT_FILENO, buf, n); // write n bytes directly to stdout fd

    if (n < 0) perror("cat: read");
    close(fd);
}

void cmd_uptime(void)
{
    char buf[256];

    // open /proc/uptime
    int fd = open("/proc/uptime", O_RDONLY); 
    if (fd < 0) {perror("uptime: open /proc/uptime"); return;}
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n < 0) {perror("uptime: read"); return;}
    buf[n] = '\0';

    double up_sec;
    sscanf(buf, "%lf", &up_sec); // parse uptime seconds

    long up = (long)up_sec;
    printf("Uptime: %02ld:%02ld:%02ld\n", up / 3600, (up % 3600) / 60, up % 60);

    // open /proc/loadavg
    fd = open("/proc/loadavg", O_RDONLY);
    if (fd < 0) { perror("uptime: open /proc/loadavg"); return; }
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n < 0) { perror("uptime: read loadavg"); return; }
    buf[n] = '\0';

    double la1, la5, la15;
    sscanf(buf, "%lf %lf %lf", &la1, &la5, &la15); // parse load averages
    printf("Load average: %.2f (1m)  %.2f (5m)  %.2f (15m)\n", la1, la5, la15);
}

void cmd_meminfo(void)
{
    // open /proc/meminfo
    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) {perror("meminfo: open"); return;}

    char buf[BUF_SIZE];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n < 0) {perror("meminfo: read"); return;}
    buf[n] = '\0';

    const char *fields[] = {"MemTotal:", "MemFree:", "MemAvailable:", "Buffers:", "Cached:", NULL};

    for (int i = 0; fields[i] != NULL; i++)
    {
        char *pos = strstr(buf, fields[i]); //find the field label in the buffer
        if (!pos) continue;
        char *end = strchr(pos, '\n');
        if (end) *end = '\0';
        printf("%s\n", pos);
        if (end) *end = '\n'; //restore the newline
    }
}

void runSingleCommand(char **av, int ac)
{
    if (ac == 0 || av[0] == NULL) return;   

    if (strcmp(av[0], "exit") == 0)
    {
        int status = 0; 
        if (ac > 1) 
        {
            char *endptr; // pointer for invalid trailing characters
            errno = 0; // reset errno
            long val = strtol(av[1], &endptr, 10);
            if (errno != 0 || *endptr != '\0') // check if conversion was valid
            {
                fprintf(stderr, "exit: numeric argument required\n"); 
                return; 
            }
            status = (int)val;
        }
        exit(status);
    }
    else if (strcmp(av[0], "pwd") == 0) {cmd_pwd(); return;}
    else if (strcmp(av[0], "cd") == 0) {cmd_cd(); return;}
    else if (strcmp(av[0], "echo") == 0) {cmd_echo(); return;}
    else if (strcmp(av[0], "help") == 0) {cmd_help(); return;}
    else if (strcmp(av[0], "history") == 0) {cmd_history(); return;}
    else if (strcmp(av[0], "ls") == 0) {cmd_ls(); return;}
    else if (strcmp(av[0], "whoami") == 0) {cmd_whoami(); return;}
    else if (strcmp(av[0], "uname") == 0) {cmd_uname(); return;}
    else if (strcmp(av[0], "mkdir") == 0) {cmd_mkdir(); return;}
    else if (strcmp(av[0], "touch") == 0) {cmd_touch(); return;}
    else if (strcmp(av[0], "rm") == 0) {cmd_rm(); return;}
    else if (strcmp(av[0], "cp") == 0) {cmd_cp(); return;}
    else if (strcmp(av[0], "mv") == 0) {cmd_mv(); return;}
    else if (strcmp(av[0], "cat") == 0) {cmd_cat(); return;}
    else if (strcmp(av[0], "uptime") == 0) {cmd_uptime(); return;}
    else if (strcmp(av[0], "meminfo") == 0) {cmd_meminfo(); return;}

    pid_t pid = fork(); // create a child process

    if (pid == 0)
    {
        execvp(av[0], av);
        perror("Command failed"); 
        exit(1);
    }
    else if (pid > 0)
    {
        int status; 
        pid_t wpid;
        do
        {
            wpid = waitpid(pid, &status, 0); // wait for child process to finish
        } while (wpid == -1 && errno == EINTR); // retry if interrupted by signal
    }
    else perror("Fork failed");
}

int tokenize(char *str, char **av)
{
    int ac = 0;
    av[ac] = strtok(str, " ");               
    while (av[ac] != NULL && ac < MAX_ARG - 1)
    {
        ac++;
        av[ac] = strtok(NULL, " ");
    }
    return ac;
}

int parsePipe(char *input, char **left, char **right)
{
    char *pipe_pos = strchr(input, '|'); // find the pipe character 
    if (!pipe_pos) return 0;                

    *pipe_pos = '\0'; // null-terminate the left side 
    pipe_pos++; 

    while (*pipe_pos == ' ') pipe_pos++; // skip leading spaces on right side

    *left  = input;                            
    *right = pipe_pos;                        
    return 1;
}

void executePiped(char **args1, char **args2)
{
    int pipefd[2];
    pipe(pipefd); //create the kernel pipe 

    pid_t p1 = fork();

    if (p1 == 0)                              
    {
        dup2(pipefd[1], STDOUT_FILENO); // redirect stdout to pipe write end
        close(pipefd[0]); // close unused read end
        close(pipefd[1]); // close original write fd

        execvp(args1[0], args1);              
        perror("Pipe cmd1 failed");
        exit(1);
    }

    pid_t p2 = fork();

    if (p2 == 0)
    {
        dup2(pipefd[0], STDIN_FILENO); // redirect stdin to pipe read end
        close(pipefd[1]); // close unused write end
        close(pipefd[0]); // close original read fd

        execvp(args2[0], args2);              
        perror("Pipe cmd2 failed");
        exit(1);
    }

    close(pipefd[0]); 
    close(pipefd[1]); 

    int s1, s2;
    while (waitpid(p1, &s1, 0) == -1 && errno == EINTR); 
    while (waitpid(p2, &s2, 0) == -1 && errno == EINTR);
}

void dispatchCommand(void)
{
    char *left_raw  = NULL;
    char *right_raw = NULL;

    char cmd_copy[MAX_SIZE];
    strcpy(cmd_copy, command); 

    if (parsePipe(cmd_copy, &left_raw, &right_raw))
    {
        // pipe path
        char *left_av[MAX_ARG];
        int left_ac  = tokenize(left_raw,  left_av);  // tokenize left side

        char *right_av[MAX_ARG];
        int right_ac = tokenize(right_raw, right_av); // tokenize right side

        if (left_ac == 0 || right_ac == 0)
        {
            fprintf(stderr, "syntax error: empty command on one side of '|'\n");
            return;
        }

        executePiped(left_av, right_av);
    }
    else
    {
        // single-command
        parseCommand();
        runSingleCommand(argv, argc);
    }
}

int main(void)
{
    if (getcwd(rootDir, sizeof(rootDir)) == NULL) // capture launch dir as root
    {
        perror("main: getcwd");
        return 1;
    }

    while (1)                               
    {
        printDir();                           
        fflush(stdout); // flush so prompt appears before blocking

        takeCommand();
        addHistory(command);
        dispatchCommand();                     
    }

    return 0;
}
