CC = gcc
options = -Wall -g -std=c99 -O2 -fsanitize=address,undefined

all: mysh clean

alstr.o: alstr.c alstr.h
	$(CC) $(options) -c alstr.c

alchar.o: alchar.c alchar.h
	$(CC) $(options) -c alchar.c

shell.o: shell.c shell.h alstr.h alchar.h
	$(CC) $(options) -c shell.c

mysh: alstr.o alchar.o shell.o
	$(CC) $(options) shell.o alstr.o alchar.o -o mysh


debug.o: shell.c shell.h alstr.h alchar.h
	$(CC) $(options) -c -DDEBUG shell.c -o debug.o
debug: alstr.o alchar.o debug.o 
	$(CC) $(options) debug.o alstr.o alchar.o -o debug

clean:
	rm -f *.o