AUTHORS: Darshan Lakshminarayanan and Sivaragha Deepika Buddana
NetIds: DL1058 and SDB210

We collaborated on this project by sharing a document and pasting our code.

---------------------------------------------USAGE---------------------------------------------
if mysh executable is not in directory type:
    make

Interactive mode:
    ./mysh

Batch mode:
    ./mysh test.sh

Batch mode (alternate modes):
    cat test.sh | ./mysh
    ./mysh < test.sh

Interactive mode activates an interactive shell

Batch mode executes all programs in the file given by file.txt

--------------------------------------------FEATURES-------------------------------------------

cd, pwd, which, and exit are built-in commands

other commands are searched for in /usr/local/bin, /usr/bin, /bin in that order 
    ls 
    grep
    nano
    vim

direct paths beginning with '/' will be executed

input and output can be redirected with < and > respectively
    ls > out.txt 
    grep world < in.txt > out.txt 

any number of processes can be piped together
    ls | grep out.txt
    cat in.txt | grep hello | grep world

wildcard expansion with '*' is available
    echo test/*.txt

some sample commands:
    cat in.txt | grep hello | grep world | exit > exit.txt
    ls | grep bye < in.txt | exit bye bye
    ls test/*.txt | grep one.txt > out.txt 