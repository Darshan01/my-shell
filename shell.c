#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <strings.h>
#include <string.h>
#include <glob.h>

#include "shell.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#define BUFLENGTH 1

int interactive = 0; //interactive or batch mode

int main(int argc, char* argv[]){

    //detrmine if file was passed for batch mode
    if(argc == 2){
        int in = open(argv[1], O_RDONLY);
        if(in < 0) {
            fprintf(stderr, "mysh: %s: No such file\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        dup2(in, STDIN_FILENO);
    }

    //too many arguments
    if(argc > 2){
        fprintf(stderr, "mysh takes 0 arguments or 1 argument\nusage: ./mysh [input file]\n");
        exit(EXIT_FAILURE);
    }

    //determine if STDIN is the terminal, if it is, interactive mode = 1
    if(isatty(STDIN_FILENO)){
        printf("Welcome to my shell!\n");
        interactive = 1;
    }

    //arraylist to hold our line
    alCH_t *line = malloc(sizeof(alCH_t));
    alch_init(line, BUFLENGTH);

    //keep track of if last operation succeeded or failed
    int lastOpReturned = 0;

    //main loop for reading input
    while(1){

        if(interactive) printf("mysh> ");

        //force output before we start reading from the terminal
        fflush(stdout);

        //call readline, program is held here until we detect a new line
        int bytesRead = readline(line);
        
        //stop program if at end of file in batch mode
        if(!interactive) if(bytesRead == 0) break;

        //arraylist to hold commands separated by pipes
        alSTR_t commands;
        alstr_init(&commands, 2);

        //keep track of start of each command
        int start = 0;

        //integer array of pipe files
        int *pipes;

        //separate by pipe character
        for(int i = 0; i < line->pos; i++){
            if(line->data[i] == '|'){
                line->data[i] = '\0';
                alstr_push(&commands, line->data + start);
                start = i + 1;
            }
        }

        alstr_push(&commands, line->data + start);

        //keep track of input and output files given by redirects
        alCH_t inFile;
        alCH_t outFile;
        
        //malloc space for pipes array
        pipes = malloc(2 * (commands.length - 1) * sizeof(int));

        //create each pipeline with pipe()
        if(commands.length > 1){
            for(int i = 0; i < commands.length - 1; i++){
                if(pipe(pipes + 2*i) < 0) fprintf(stderr, "pipe failed\n");
            }
        }

        
        alSTR_t arglist; //arraylist for array of arguments
        char *token; //pointer to hold tokens
        int numForks = 0; //count number of forks to determine number of times to call wait()

        //execute each process
        for(int i = 0; i < commands.length; i++){
            
            //intialize all arraylists
            alstr_init(&arglist, 10);
            alch_init(&inFile, 64);
            alch_init(&outFile, 64);

            token = strtok(commands.data[i], " "); //tokenize by space
            
            //empty line
            if(token == NULL) {
                alstr_destroy(&arglist);
                alch_destroy(&inFile);
                alch_destroy(&outFile);
                break;
            } 

            //determine if previous token was < or >
            int outReDirect = 0;
            int inReDirect = 0;

            //determine if first token is then or else
            int then = 0;
            int els = 0;

            if(strcmp(token, "then") == 0){
                then = 1;
                token = strtok(NULL, " ");
            } else if(strcmp(token, "else") == 0){
                els = 1;
                token = strtok(NULL, " ");
            }

            //stores tokens separately, takes care of redirects
            while(token != NULL){

                //in batch mode if line is comment
                if(!interactive){
                    if(token[0] == '#'){
                        token = strtok(NULL, " ");
                        continue;
                    }
                }

                WHILE:

                //first character in token is a redirect character
                if(token[0] == '>'){

                    //token starts with >
                    //read rest of token as output file
                    if(strlen(token) != 1){
                        token += 1;
                        outReDirect = 1;
                        continue;
                    }

                    //token is >
                    //read the next token as the output file
                    token = strtok(NULL, " ");
                    outReDirect = 1;
                    continue;

                //first character is <, repeat all steps as above
                } else if(token[0] == '<'){

                    if(strlen(token) != 1){
                        token += 1;
                        inReDirect = 1;
                        continue;
                    }
                    token = strtok(NULL, " ");
                    inReDirect = 1;
                    continue;

                //first character is not < or >
                } else if(token[0] != '<' && token[0] != '>'){

                    //loop through token to search for < or >
                    for(int j = 1; j < strlen(token) - 1; j++){

                        //if token has < in it
                        if(token[j] == '<'){
                            token[j] = '\0';

                            //if previous token had <
                            if(outReDirect){
                                //create and close the file
                                int f = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0640);
                                close(f);

                                //reset the arraylist holding the outfile
                                alch_destroy(&outFile);
                                alch_init(&outFile, 64);

                                //store the token
                                alch_pushStr(&outFile, token);
                                outReDirect = 0;

                            //if previous token was <
                            } else if(inReDirect){

                                //reset arraylist and store input file
                                alch_destroy(&inFile);
                                alch_init(&inFile, 64);
                                alch_pushStr(&inFile, token);
                                inReDirect = 0;

                            } else {
                                //token is an argument, expand * and push into argument list
                                expandAndPush(&arglist, token);
                            }

                            //token starts at character after <
                            //go to top of while loop
                            token += (j + 1);
                            inReDirect = 1;
                            goto WHILE;

                        //same steps as above but with >
                        } else if(token[j] == '>'){
                            token[j] = '\0';
                            
                            if(outReDirect){
                                int f = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0640);
                                close(f);
                                alch_destroy(&outFile);
                                alch_init(&outFile, 64);
                                alch_pushStr(&outFile, token);
                                outReDirect = 0;

                            } else if(inReDirect){
                                alch_destroy(&inFile);
                                alch_init(&inFile, 64);
                                alch_pushStr(&inFile, token);
                                inReDirect = 0;
                            } else {
                                expandAndPush(&arglist, token);
                            }
                            
                            token += (j + 1);
                            outReDirect = 1;
                            goto WHILE;

                        }
                    }

                    //last character is redirect
                    if(token[strlen(token) - 1] == '>'){
                        token[strlen(token) - 1] = '\0';

                        //save current token without >
                        if(outReDirect){
                            int f = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0640);
                            close(f);
                            alch_destroy(&outFile);
                            alch_init(&outFile, 64);
                            alch_pushStr(&outFile, token);
                            outReDirect = 0;

                        } else if(inReDirect){
                            alch_destroy(&inFile);
                            alch_init(&inFile, 64);
                            alch_pushStr(&inFile, token);
                            inReDirect = 0;
                        } else {
                            expandAndPush(&arglist, token);
                        }

                        outReDirect = 1;

                        //go to next token, put it through the previous checks
                        token = strtok(NULL, " ");
                        continue;

                    } 
                    //same steps as above but last character is <
                    else if(token[strlen(token) - 1] == '<'){
                        token[strlen(token) - 1] = '\0';

                        if(outReDirect){
                            int f = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0640);
                            close(f);
                            alch_destroy(&outFile);
                            alch_init(&outFile, 64);
                            alch_pushStr(&outFile, token);
                            outReDirect = 0;

                        } else if(inReDirect){
                            alch_destroy(&inFile);
                            alch_init(&inFile, 64);
                            alch_pushStr(&inFile, token);
                            inReDirect = 0;

                        } else {
                            expandAndPush(&arglist, token);
                        }

                        inReDirect = 1;

                        token = strtok(NULL, " ");
                        continue;

                    }

                    //token does not have < or >
                    if(outReDirect){
                        int f = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0640);
                        close(f);
                        alch_destroy(&outFile);
                        alch_init(&outFile, 64);
                        alch_pushStr(&outFile, token);
                        outReDirect = 0;

                    } else if(inReDirect){
                        alch_destroy(&inFile);
                        alch_init(&inFile, 64); 
                        alch_pushStr(&inFile, token);
                        inReDirect = 0;

                    } else {
                        expandAndPush(&arglist, token);
                    }
                }

                //go to next token
                token = strtok(NULL, " ");
            }
            
            //no command to run
            if(arglist.length == 0) {
                alstr_destroy(&arglist);
                alch_destroy(&inFile);
                alch_destroy(&outFile);
                break;
            }

            //then conditional
            if(then){
                //last operation was not successful, break
                if(lastOpReturned != EXIT_SUCCESS){
                    alstr_destroy(&arglist);
                    alch_destroy(&inFile);
                    alch_destroy(&outFile);
                    lastOpReturned = EXIT_FAILURE;
                    break;
                }
            }

            //else conditional
            if(els){
                //last operation was successful, break
                if(lastOpReturned == EXIT_SUCCESS){
                    alstr_destroy(&arglist);
                    alch_destroy(&inFile);
                    alch_destroy(&outFile);
                    lastOpReturned = EXIT_SUCCESS;
                    break;
                }
            }

            //null terminate the argument list
            alstr_push(&arglist, NULL);

            //tracks existence of input and output files
            int in = -1;
            int out = -1;

            //store original stdout and stdin
            int stdOut = -1;
            int stdIn = -1;

            //exit shell
            if(strcmp(arglist.data[0], "exit") == 0){
                //wait for any previous child processes
                if(commands.length > 1) wait(&lastOpReturned);
                
                //if input file was stored, change stdin
                if(inFile.length > 0){
                    stdIn = dup(STDOUT_FILENO);
                    in = open(inFile.data, O_RDONLY);
                    if(in != -1) dup2(in, STDIN_FILENO);
                    else {
                        fprintf(stderr, "mysh: %s: No such file\n", inFile.data);
                    }
                    close(in);
                } 
                //no redirect, check if there are pipes
                else if(commands.length > 1){

                    //not first argument, use pipe file ast input
                    if(i != 0){
                        stdIn = dup(STDOUT_FILENO);
                        dup2(pipes[2*i - 2], STDIN_FILENO);
                    }
                }

                //do the same as above but with redirect output
                if(outFile.length > 0){
                    fflush(stdout);
                    stdOut = dup(STDOUT_FILENO);
                    out = open(outFile.data, O_CREAT | O_WRONLY | O_TRUNC, 0640);
                    dup2(out, STDOUT_FILENO);
                    close(out);
                }
                //close all pipe files
                for(int j = 0; j < 2 * (commands.length - 1); j++){
                    close(pipes[j]);
                }
                //print arguments passed to exit
                for(int j = 1; j < arglist.length - 1; j++){
                    printf("%s ", arglist.data[j]);
                }

                //arraylist to store the data from piped or redirected stdin
                alCH_t exitPrint;
                alch_init(&exitPrint, BUFLENGTH);

                //if we have an input file
                if(in != -1 || (commands.length > 1 && i != 0)){
                    //read all of stdin
                    char buf[BUFLENGTH + 1];
                    int numRead = 1;
                    while(numRead > 0){
                        numRead = read(STDIN_FILENO, buf, BUFLENGTH);
                        buf[numRead] = '\0';
                        alch_pushStr(&exitPrint, buf);
                    }
                }

                //if input file was not empty, print its contents
                if(exitPrint.length != 0){
                    if(arglist.length > 2) printf("\n");
                    printf("%s", exitPrint.data);
                }

                //formatting
                if(arglist.length > 2 && exitPrint.length == 0) printf("\n");
                
                alch_destroy(&exitPrint); //destroy arraylist

                //close out file if it was opened
                if(out != -1){
                    fflush(stdout);
                    dup2(stdOut, STDOUT_FILENO);
                    close(stdOut);
                }

                //close input files if opened
                if(in != -1 || commands.length > 1){
                    dup2(stdIn, STDIN_FILENO);
                    close(stdIn);
                }

                if(interactive) printf("\nExiting my shell.\n");

                //free all data
                free(pipes);
                alstr_destroy(&arglist);
                alch_destroy(&inFile);
                alch_destroy(&outFile);
                alstr_destroy(&commands);
                alch_destroy(line);
                free(line);
                exit(EXIT_SUCCESS);    
            } 
            
            //cd
            else if(strcmp(arglist.data[0], "cd") == 0){

                //wrong number of arguments
                if(arglist.length != 3){
                    fprintf(stderr, "cd takes one argument, received %i.\n", arglist.length-2);
                    lastOpReturned = EXIT_FAILURE;
                    alstr_destroy(&arglist);
                    alch_destroy(&inFile);
                    alch_destroy(&outFile);
                    break;
                }
                
                //change directory, check for errors
                if(chdir(arglist.data[1]) != 0){
                    fprintf(stderr, "cd failed.\n");
                    lastOpReturned = EXIT_FAILURE;
                    alstr_destroy(&arglist);
                    alch_destroy(&inFile);
                    alch_destroy(&outFile);
                    break;
                }

            } 
            
            //any other command
            else {
                //increase the number of forks
                numForks++;
                int pid = fork(); //fork

                //fork failed
                if(pid < 0){
                    fprintf(stderr, "fork failed\n");
                    exit(EXIT_FAILURE);
                }

                //inside child process
                if(pid == 0){
                    
                    //check if there is a redirect input file
                    if(inFile.length > 0){
                        int in = open(inFile.data, O_RDONLY);

                        if(in < 0){
                            fprintf(stderr, "mysh: %s: No such file\n", inFile.data);
                            alstr_destroy(&arglist);
                            alch_destroy(&inFile);
                            alch_destroy(&outFile);
                            alstr_destroy(&commands);
                            alch_destroy(line);
                            free(line);
                            free(pipes);
                            exit(EXIT_FAILURE);
                        } //file not found
                        
                        dup2(in, STDIN_FILENO);
                    } 

                    //check if there are pipes
                    else if(commands.length > 1){
                        //if not first command, read input from pipe
                        if(i != 0){
                            dup2(pipes[2*i - 2], STDIN_FILENO);
                        }
                    }
                    
                    //repeat above steps for output files
                    if(outFile.length > 0){
                        int out = open(outFile.data, O_CREAT | O_WRONLY | O_TRUNC, 0640);
                        dup2(out, STDOUT_FILENO);
                    } 
                                    
                    else if(commands.length > 1){
                        if(i != commands.length - 1){
                            dup2(pipes[2*i + 1], STDOUT_FILENO);
                        }
                    }

                    //close all pipes
                    for(int j = 0; j < 2 * (commands.length - 1); j++){
                        close(pipes[j]);
                    }

                    //execute command
                    int status = execute(&arglist);
                    alstr_destroy(&arglist);
                    alch_destroy(&inFile);
                    alch_destroy(&outFile);
                    alstr_destroy(&commands);
                    alch_destroy(line);
                    free(line);
                    free(pipes);
                    exit(status);
                }

                //close input and outpuf files if opened
                if(in != -1) close(in);
                if(out != -1) close(out);
            }
            

            alstr_destroy(&arglist);
            alch_destroy(&inFile);
            alch_destroy(&outFile);
        }
        
        //close all pipes
        for(int i = 0; i < 2 * (commands.length - 1); i++){
            close(pipes[i]);
        }

        
        //wait for all forked children
        for(int i = 0; i < numForks; i++){
            wait(&lastOpReturned);
        }

        //free pipe array and destroy arraylist
        free(pipes);
        alstr_destroy(&commands);

        //move line buffer
        if(!interactive){
            if((line->length - line->pos - 1) >= 0) alch_move(line);
            if(line->length == line->pos) line->length--;
        } else {
            alch_move(line);
        }

    }

    alch_destroy(line);
    free(line);
}

//expand glob and push
int expandAndPush(alSTR_t *arglist, char* item){
    //determine if glob in token
    int numGlobs = 0;
    for(int i = 0; i < strlen(item); i++){
        if(item[i] == '*' && (i == 0 || (i != 0 && item[i-1] !='\\'))){
            numGlobs = 1;
            break;
        }
    }
    
    //if globs were found
    if(numGlobs){
        
        //expand glob with glob()
        char **items;
        glob_t globs;
        int globstatus = glob(item, GLOB_NOCHECK, NULL, &globs); //glob: GLOB_NOCHECK returns original pattern if no matches found
        if(globstatus == GLOB_ABORTED || globstatus == GLOB_NOSPACE){
            fprintf(stderr, "Error expanding * in %s\n", item);
            globfree(&globs);
            alstr_push(arglist, item); //push original item if glob broke
            return 1;
        }

        //iterate through items found by glob() and push them to arglist
        items = globs.gl_pathv;
        while(*items != NULL){
            alstr_push(arglist, *items);
            items++;
        }

        //free glob struct
        globfree(&globs);
        return 0;
    }
    
    alstr_push(arglist, item);
    return 0;
}

//code to read line from terminal or batch file
int readline(alCH_t *line){
    
    int bytesRead = 0; //number returned by read()

    while((bytesRead = read(STDIN_FILENO, line->data + line->length, line->capacity - line->length))){
        
        if(bytesRead == -1){
            fprintf(stderr, "There was an error reading input\n");
            exit(EXIT_FAILURE);
        }

        line->length += bytesRead; //line length increment by bytes read

        //iterate until newline is found or end of bytes read
        while(line->pos != line->length && line->data[line->pos] != '\n'){
            line->pos++;
        }

        //if line is at full capacity, resize
        if(line->pos == line->capacity){
            alch_resize(line);
            return readline(line);
        }

        //if new line found, start executing command
        if(line->data[line->pos] == '\n'){
            line->data[line->pos] = '\0';
            return bytesRead;
        }
    }

    //end of file

    //in interactive mode if there is no new line, read again
    if(interactive){
        if(line->data[line->pos] != '\n'){
            return readline(line);
        }
    } 
    
    //in batch mode, read through rest of buffer and find all lines
    else {
        while(line->pos < line->length && line->data[line->pos] != '\n'){
            line->pos++;
        } 
        line->data[line->pos] = '\0';
        if(line->pos <= line->length) return 1;
    }
    
    return bytesRead;
}

//find path to given program
int which(char *program, alCH_t *path){

    //store current directory for later
    char *curDir = getcwd(NULL, 0);
    if(chdir("/usr/local/bin/") < 0) fprintf(stderr, "Could not open path /usr/local/bin\n"); //change directory to first spot to check

    //create path to the program
    alch_pushStr(path, "/usr/local/bin/");
    alch_pushStr(path, program);

    //check if file exists
    if(access(path->data, F_OK) == 0){
        if(chdir(curDir)) fprintf(stderr, "Could not return to path: %s\n", curDir); //return to working directory
        free(curDir);
        return 0;
    }

    //destroy and recreate arraylist
    alch_destroy(path);
    alch_init(path, 32);

    //re-do above steps with /usr/bin
    if(chdir("/usr/bin/") < 0) fprintf(stderr, "Could not open path /usr/bin\n");
    alch_pushStr(path, "/usr/bin/");
    alch_pushStr(path, program);

    if(access(program, F_OK) == 0) {
        if(chdir(curDir) < 0) fprintf(stderr, "Could not return to path: %s\n", curDir);;
        free(curDir);
        return 0;
    }

    alch_destroy(path);
    alch_init(path, 32);

    //re-do above steps with /bin
    if(chdir("/bin/") < 0) fprintf(stderr, "coudl not open path /bin");
    alch_pushStr(path, "/bin/");
    alch_pushStr(path, program);

    if(access(program, F_OK) == 0) {
        if(chdir(curDir) < 0) fprintf(stderr, "Could not return to path: %s\n", curDir);;
        free(curDir);
        return 0;
    }

    if(chdir(curDir) < 0) fprintf(stderr, "Could not return to path: %s\n", curDir);
    free(curDir);

    return -1; //program not found
}

//general execution function
int execute(alSTR_t *args){

    //pwd implementation
    if(strcmp(args->data[0], "pwd") == 0){
        
        char *buf = getcwd(NULL, 0);

        if(buf == NULL){
            fprintf(stderr, "Could not obtain current directory.\n");
        }

        printf("%s\n", buf);
        free(buf);
        return EXIT_SUCCESS;

    } 
    
    //which implementation
    else if(strcmp(args->data[0], "which") == 0){

        //wrong number of arguments
        if(args->length != 3){
            fprintf(stderr, "which takes one argument, received %i.\n", args->length-2);
            return EXIT_FAILURE;
        }

        //built in commands
        if(strcmp(args->data[1], "pwd") == 0 || strcmp(args->data[1], "which") == 0 || strcmp(args->data[1], "cd") == 0 || strcmp(args->data[1], "exit") == 0) return 1;
        
        alCH_t path;
        alch_init(&path, 32);

        int ret = which(args->data[1], &path);
        if(ret != -1){
            printf("%s\n", path.data); 
            alch_destroy(&path);
            return EXIT_SUCCESS;
        }

        alch_destroy(&path);
        return EXIT_FAILURE;
    }

    //calling any executable given by path
    else if(args->data[0][0] == '/'){

        //execute file
        int flag = execv(args->data[0], (args->data + 1));

        //error executing given file
        if(flag == -1){
            fprintf(stderr, "Program could not be executed: %s\n", args->data[0]);
            return EXIT_FAILURE;
        }

        return flag;
    }

    //bare names
    else {

        //find file in given directories
        alCH_t path;
        alch_init(&path, 32);

        int ret = which(args->data[0], &path);

        //if program was found
        if(ret != -1){

            //execute program
            int flag = execv(path.data, (args->data));
            alch_destroy(&path);

            if(flag == -1){
                fprintf(stderr, "Program could not be executed: %s\n", path.data);
                return EXIT_FAILURE;
            }
            return flag;
        }

        alch_destroy(&path);
    }

    //program not found in directories
    fprintf(stderr, "Command not found: %s\n", args->data[0]);
    return EXIT_FAILURE;
}