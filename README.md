# Custom Linux Shell (`myshell`)

## 1. Compilation Steps

To compile the shell using `gcc` with all compiler warnings enabled:

```bash
gcc -Wall -o myshell main.c
```

To run the shell interactively:

```bash
./myshell
```

Alternatively, run the automated compilation and test script:

```bash
bash build_and_test.sh
```

---

## 2. Supported Features

- **Interactive Command Loop**: Displays a prompt showing the current working directory and accepts user input continuously until `exit` or `Ctrl+D` (EOF).
- **Multiple Argument Parsing**: Splits user input into individual command tokens, supporting flags, paths, and arbitrary argument counts.
- **Root Directory Lock**: Restricts directory navigation (`cd`) and all file operations within the initial launch folder to prevent unauthorized filesystem access.
- **Directory Operations**: Listing contents (`ls`, `ls -a`, `ls -l`, `ls [dir]`), directory creation (`mkdir`), and empty directory deletion (`rm`).
- **File Management**: File creation and timestamp updates (`touch`), file copying with self-copy detection (`cp`), file moving and renaming (`mv`), file reading (`cat`), and file deletion (`rm`).
- **System & User Information**: Printing working directory (`pwd`), active username (`whoami`), system/kernel architecture (`uname`), uptime and load average (`uptime`), and RAM metrics (`meminfo`).
- **Text & Variable Printing**: Printing text arguments with `$VAR` environment variable expansion (`echo`).
- **Command Piping (`|`)**: Inter-process communication routing standard output of the first command directly to standard input of the second command using kernel pipes.
- **Command History (`history`)**: Fixed-size circular buffer recording up to the last 50 session commands with chronological numbering.
- **External Program Execution**: Forking child processes to execute external system binaries found in `PATH` with signal-safe waiting.
- **Process Termination**: Exiting the shell session with an optional numeric exit status code (`exit [code]`).

---

## 3. System Calls Utilized for Each Feature

| Feature / Command | Description | System Calls & Library Functions Utilized |
|---|---|---|
| `pwd` | Print current working directory | `getcwd()` |
| `cd [dir]` | Change directory (confined to root) | `chdir()`, `getcwd()`, `realpath()` |
| `ls [-l] [-a] [dir]` | List directory contents | `opendir()`, `readdir()`, `closedir()`, `stat()`, `lstat()`, `realpath()` |
| `whoami` | Display active user name | `getuid()`, `getpwuid()` |
| `uname` | Display OS and kernel architecture | `uname()` |
| `mkdir <dir>` | Create new directory | `mkdir()`, `realpath()` |
| `touch <file>` | Create empty file or update timestamp | `open()`, `close()`, `utimensat()`, `realpath()` |
| `rm <path>` | Remove regular file or empty directory | `stat()`, `unlink()`, `rmdir()`, `realpath()` |
| `cp <src> <dst>` | Copy file byte-by-byte (same-file safe) | `stat()`, `open()`, `read()`, `write()`, `close()`, `realpath()` |
| `mv <src> <dst>` | Move or rename file/directory | `rename()`, `realpath()` |
| `cat <file>` | Stream file content to stdout | `open()`, `read()`, `write()`, `close()`, `realpath()` |
| `echo [args]` | Print text and expand `$VAR` variables | `getenv()` |
| `uptime` | Display system uptime and load average | `open()`, `read()`, `close()` (reads `/proc/uptime`, `/proc/loadavg`) |
| `meminfo` | Display system memory statistics | `open()`, `read()`, `close()` (reads `/proc/meminfo`) |
| `history` | Display numbered session history | In-memory circular array (no system calls required) |
| `help` | Display command usage instructions | Formatted output (no system calls required) |
| `cmd1 \| cmd2` | Pipe stdout of `cmd1` to stdin of `cmd2` | `pipe()`, `dup2()`, `fork()`, `execvp()`, `close()`, `waitpid()` |
| External Programs | Execute external commands via `PATH` | `fork()`, `execvp()`, `waitpid()` |
| `exit [code]` | Exit shell with optional status code | `strtol()`, `exit()` |

---

## 4. Test Cases and Outcomes

The automated test script (`build_and_test.sh`) compiles the shell with `gcc -Wall -o myshell main.c` and feeds test commands directly via standard input.

| # | Test Command | Feature / Category | Expected Outcome | Actual Result |
|---|---|---|---|---|
| 1 | `help` | Help menu | Prints list of supported commands and syntax | PASS: Displayed command guide |
| 2 | `pwd` | Working directory | Prints absolute path of current folder | PASS: Displayed current directory path |
| 3 | `whoami` | User identification | Prints current logged-in username | PASS: Displayed `ohin` |
| 4 | `uname` | Kernel / OS info | Displays OS name, kernel version, machine architecture | PASS: Displayed `Linux ... x86_64` |
| 5 | `ls` | Directory listing | Lists non-hidden files in current directory | PASS: Entries listed in short format |
| 6 | `ls -a` | Hidden files | Lists all entries including `.` and `..` | PASS: Hidden entries listed |
| 7 | `ls -l` | Long format listing | Displays permissions (`drwxr-xr-x`), sizes, dates, names | PASS: Formatted file list displayed |
| 8 | `mkdir test_dir` | Directory creation | Creates new directory `test_dir` with permissions `0755` | PASS: `test_dir` created |
| 9 | `ls` | Directory verification | Verifies `test_dir` appears in directory listing | PASS: `test_dir` listed |
| 10 | `touch test_dir/hello.txt` | File creation | Creates empty file inside subdirectory | PASS: `hello.txt` created |
| 11 | `ls test_dir` | Targeted listing | Lists entries inside `test_dir` | PASS: Lists `hello.txt` |
| 12 | `ls -l test_dir` | Targeted long listing | Long format metadata for entries inside `test_dir` | PASS: Formatted metadata displayed |
| 13 | `echo Hello World` | Text output | Prints string arguments to standard output | PASS: Output `Hello World` |
| 14 | `echo $HOME` | Variable expansion | Expands environment variable `$HOME` | PASS: Output `/home/ohin` |
| 15 | `cp test_dir/hello.txt test_dir/hello_copy.txt` | File copy | Copies file byte-by-byte | PASS: `hello_copy.txt` created |
| 16 | `ls test_dir` | Copy verification | Confirms both `hello.txt` and `hello_copy.txt` exist | PASS: Both files listed |
| 17 | `cp test_dir/hello.txt test_dir/hello.txt` | Same-file protection | Detects matching inode/device; halts without truncating | PASS: "are the same file" error printed |
| 18 | `mv test_dir/hello_copy.txt test_dir/hello_moved.txt` | Move / rename | Renames file atomically | PASS: `hello_moved.txt` created |
| 19 | `ls test_dir` | Rename verification | Confirms `hello_copy.txt` was renamed to `hello_moved.txt` | PASS: `hello_moved.txt` listed |
| 20 | `cat test_msg.txt` | File reading | Streams file contents to stdout | PASS: Output `Hello, custom shell file test!` |
| 21 | `cat test_dir/hello.txt` | Empty file reading | Reads empty (0-byte) file | PASS: Empty output without error |
| 22 | `cat /etc/passwd` | Root boundary (read) | Rejects reading files outside shell root | PASS: "access denied, outside shell root" |
| 23 | `mkdir ../outside_test` | Root boundary (write) | Rejects creating directories outside shell root | PASS: "access denied, outside shell root" |
| 24 | `uptime` | System uptime | Reads `/proc/uptime` and `/proc/loadavg` | PASS: Formatted uptime & load printed |
| 25 | `meminfo` | Memory statistics | Reads `/proc/meminfo` | PASS: MemTotal, MemFree, etc. printed |
| 26 | `echo pipeline_test \| cat` | Piping (built-in) | Streams stdout of `echo` to stdin of `cat` | PASS: Output `pipeline_test` |
| 27 | `ls \| grep main` | Piping (external) | Filters output of `ls` through `grep` | PASS: Output `main.c` |
| 28 | `history` | Command history | Displays numbered history list from circular buffer | PASS: All 28 executed commands printed |
| 29 | `rm test_msg.txt` | File deletion | Deletes test sample file via `unlink()` | PASS: File deleted |
| 30 | `rm test_dir/hello.txt` | File deletion | Deletes regular file via `unlink()` | PASS: File deleted |
| 31 | `rm test_dir/hello_moved.txt` | File deletion | Deletes second regular file via `unlink()` | PASS: File deleted |
| 32 | `ls test_dir` | Directory empty verification | Verifies `test_dir` is now empty | PASS: No files listed |
| 33 | `rm test_dir` | Directory deletion | Removes empty directory via `rmdir()` | PASS: Directory deleted |
| 34 | `ls` | Removal verification | Verifies `test_dir` was removed | PASS: `test_dir` no longer listed |
| 35 | `cd ..` | Root boundary (`cd`) | Blocks navigating above root directory | PASS: "cannot go above the shell root directory" |
| 36 | `pwd` | Root boundary verification | Verifies directory stayed at root | PASS: Remains in shell root directory |
| 37 | `exit 0` | Shell termination | Exits shell process with exit status 0 | PASS: Shell exits cleanly with code 0 |
