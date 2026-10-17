#complile
echo "=== Compiling myshell ==="
gcc -Wall -o myshell main.c
if [ $? -ne 0 ]; then
    echo "Compilation failed. Aborting tests."
    exit 1
fi
echo "Compilation successful."
echo ""

#Run automated tests
echo "Hello, custom shell file test!" > test_msg.txt

./myshell <<'EOF'
help
pwd
whoami
uname
ls
ls -a
ls -l
mkdir test_dir
ls
touch test_dir/hello.txt
ls test_dir
ls -l test_dir
echo Hello World
echo $HOME
cp test_dir/hello.txt test_dir/hello_copy.txt
ls test_dir
cp test_dir/hello.txt test_dir/hello.txt
mv test_dir/hello_copy.txt test_dir/hello_moved.txt
ls test_dir
cat test_msg.txt
cat test_dir/hello.txt
cat /etc/passwd
mkdir ../outside_test
uptime
meminfo
echo pipeline_test | cat
ls | grep main
history
rm test_msg.txt
rm test_dir/hello.txt
rm test_dir/hello_moved.txt
ls test_dir
rm test_dir
ls
cd ..
pwd
exit 0
EOF

echo ""
echo "=== All tests completed ==="

