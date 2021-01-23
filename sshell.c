#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>


//declarations
int mysyscall(char *inputcmd, int* message, int* numpipe);
char** parsecmd(char *cmd, char** storedstr, int* numargs);
bool checkmultipleargs(char *cmd);
void excenoarg(char* cmd, char** rrdc);
char* removeleadingspaces(char* cmd);
void excecutecmd(char* cmd, char** args, char** rrdc);
char** checkredirection(char* cmd);
void redirection(char* filename);
char** checkpipe(char* cmd, int* numpipe);
char* removetrailingspaces(char* str, unsigned long length);
void multpipe(char** pipe, int numpipe, int curpipe, char** rdc, int* message, int* fdarray[]);
void handlearguments(char* cmd, char** rdc);
int handlespecialcmd(char** parsedcmd, int numargs);
int setavariable(char** parsedcmd);
int onepipe(char** pipe, char** rdc);
int nonforkfunc(char* cmd);
int handleparsingerror(char** pip, const int* numpipe);
void printerrormessage();
#define CMDLINE_MAX 512
#define ExitStatusDevision 256
#define MaximunArguments 16
#define lowercaseA 97
#define lowercaseZ 122
char* storedvariable[26];

enum ParsingError{
    Notusedone,
    Notusedsecond,
    MissingCommand,
    TooManyArguements,
    MislocatedRed,
    NoOutputFile,
    CannotOpen,
    InvalidVarialbeName
};

void redirection(char* filename){ // redirect the output to the given file
    int fd;
    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    dup2(fd, STDOUT_FILENO);
    close(fd);
}


void excecutecmd(char* cmd, char** args, char** rrdc){ // execute a program in this process
    // redirect the output if needed
    if (!(strcmp(rrdc[1], ">"))){
        redirection(rrdc[2]);
    }
    execvp(cmd, args);
    fprintf(stderr, "Error: Command not found\n");
    exit(1);
}

void excenoarg(char* cmd, char** rrdc){ // execute the no argument command
    // set the default args for no argument cmd
    char *args[] = {cmd, NULL, NULL};
    excecutecmd(cmd, args, rrdc);
}

int setavariable(char** parsedcmd){
    // if the variable name is valid, store the value in a global variable
    if (strlen(parsedcmd[1]) != 1 || parsedcmd[1][0] > lowercaseZ || parsedcmd[1][0] < lowercaseA){
        fprintf(stderr, "Error: invalid variable name\n");
        return (1);
    }
    storedvariable[parsedcmd[1][0] - lowercaseA] = strdup(parsedcmd[2]);
    return (0);
}

int handlespecialcmd(char** parsedcmd, int numargs){
    for (int i = 0; i < numargs; ++i) {
        if (parsedcmd[i][0] == '$') {
            // if there is an argument start with '$', but the following content is not 'a'-'z', return error code
            if (strlen(parsedcmd[i]) != 2 || parsedcmd[i][1] > 122 || parsedcmd[i][1] < 97){
                return 1;
            }
            // if variable name valid, replace the variable name by the value of the variable
            if (storedvariable[parsedcmd[i][1]-97] == 0){
                // if the variable hasn't been set up, replace it with ""
                parsedcmd[i] = "";
            }else{
                parsedcmd[i] = strdup(storedvariable[parsedcmd[i][1]-97]);
            }
        }
    }
    return 0;
}

void handlearguments(char* cmd, char** rdc){
    int numargs = 0;
    bool multargs = checkmultipleargs(cmd);

    if (multargs){ // if there are some arguments
        // create an array of char* to store parsed string
        unsigned long init_length = strlen(cmd);
        char* storedstr[init_length];
        char** parsedcmd;

        // store the parsed cmd in the array
        parsedcmd = parsecmd(cmd, storedstr, &numargs);
        // Check if there is any arguments with '$'. If any, change them into stored value.
        handlespecialcmd(parsedcmd, numargs);
        // set the arguments
        char **args = malloc(sizeof(char*)*(numargs+1));
        for (int i = 0; i < numargs; ++i) {
            args[i] = parsedcmd[i];
        }
        args[-1] = NULL;
        // execute the cmd with the arguments
        excecutecmd(parsedcmd[0],args,rdc);
    } else{
        // execute 1 argument program
        excenoarg(cmd,rdc);
    }
}

void multpipe(char** pip, int numpipe, int curpipe, char** rdc, int message[], int* fdarray[]) {
    // handle commands when pipe line detected
    // create the pipe and fork
    int fd[2];
    pipe(fd);
    fdarray[curpipe-2] = fd;
    pid_t pid;
    pid = fork();
    if (pid > 0) { // parent
        int status;
        waitpid(pid, &status, 0);
        message[curpipe-2] = status/ExitStatusDevision;
        if (curpipe == numpipe) { // connect the first process to the pipe
            close(fdarray[curpipe-2][1]);
            dup2(fdarray[curpipe-2][0], STDIN_FILENO);
            close(fdarray[curpipe-2][0]);
            if (!(strcmp(rdc[1], ">"))){
                handlearguments(rdc[0], rdc);
            } else {
                handlearguments(pip[numpipe-1], rdc);
            }
        }
        if  (curpipe < numpipe){ // connect middle processes to two pipes, one for input, one for output
            dup2(fdarray[curpipe-2][0], STDIN_FILENO);
            dup2(fdarray[curpipe-1][1], STDOUT_FILENO);
            close(fdarray[curpipe-2][0]);
            close(fdarray[curpipe-2][1]);
            close(fdarray[curpipe-1][0]);
            close(fdarray[curpipe-1][1]);
            handlearguments(pip[curpipe-1], rdc);
        }
    } else if (pid == 0) {
        if (curpipe > 2){ // recursion if need to fork more process
            multpipe(pip, numpipe, curpipe-1, rdc, message, fdarray);
        } else { // connect the last pipe to the pipe
            close(fdarray[curpipe-2][0]);
            dup2(fdarray[curpipe-2][1], STDOUT_FILENO);
            close(fdarray[curpipe-2][1]);
            handlearguments(pip[0], rdc);
        }
    } else {
        message[numpipe-curpipe] = -1;
    }
}

int onepipe(char** pipe, char** rdc){ // handle command when no pipe line detected
    pid_t pid;
    pid = fork();

    if (pid > 0){
        int status;
        waitpid(pid, &status, 0);
        return status/ExitStatusDevision;
    } else if (pid == 0){
        // handle arguments with or without file redirectioni
        if (!(strcmp(rdc[1], ">"))){
            handlearguments(rdc[0], rdc);
        } else {
            handlearguments(pipe[0], rdc);
        }
    } else {
        return (1);
    }
    return 0;
}

char** parsecmd(char *cmd, char** storedstr, int* numargs){ // parse the cmd by white spaces
    //source code
    //https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
    char* str = cmd;
    //set deliminator as space
    char Deliminator[] = " ";
    char *ptr = strtok(str, Deliminator);

    //store every separated command into the array
    while (ptr != NULL)
    {
        storedstr[*numargs] = ptr;
        ptr = strtok(NULL, Deliminator);
        (*numargs)++;
    }
    return storedstr;
}

bool checkmultipleargs(char *cmd){
    // check if there are multiple arguments
    // similar with parsecmd function but with different arguments
    char Delimiter[] = " ";
    char duplicatedcmd[strlen(cmd)];
    strcpy(duplicatedcmd, cmd);

    char *ptr = strtok(duplicatedcmd, Delimiter);
    int i = 0;
    while(ptr != NULL)
    {
        ptr = strtok(NULL, Delimiter);
        i ++;
    }
    // if there are only 1 arguments, the loop will only loop once, in which case i will equal to 1
    if (i == 1) return false;
    else return true;
}

int nonforkfunc(char* cmd){
    int numargs = 0;
    bool multargs = checkmultipleargs(cmd);
    unsigned long init_length = strlen(cmd);
    char* storedstr[init_length];
    char** parsedcmd;

    // parse the cmd and store them in an array
    parsedcmd = parsecmd(cmd, storedstr, &numargs);
    if (multargs){
        // If cmd is set, try to set it. If failed, return error code.
        if (!strcmp(parsedcmd[0],"set")){
            return (setavariable(parsedcmd) + 1);
        } else if (!strcmp(parsedcmd[0],"cd")){
            //If cmd is cd, try to cd it. If failed, return error code.
            int chdirresult = chdir(parsedcmd[1]);
            if (chdirresult == -1) {
                fprintf(stderr, "Error: Cannot cd into directory\n");
                return (2);
            }
            return (1);
        }
    } else{
        // apply pwd directly if cmd is pwd
        if (!strcmp(parsedcmd[0],"pwd")){
            printf("%s\n", get_current_dir_name());
            return (1);
        } else if (!strcmp(parsedcmd[0],"set")){
            // if cmd is set with no argument, return error code
            fprintf(stderr, "Error: invalid variable name\n");
            return (2);
        }
    }
    return 0;
}

int handleparsingerror(char** pip, const int* numpipe){ // handle paring errors in the command
    int statreturn;
    struct stat stats;
    char** rdc;
    char** mrdc;
    int numargs = 0;
    // iterate through each pipe separated command
    for (int i = 0; i < *numpipe; i++){
        if (!(strcmp(pip[i], ""))){
            return MissingCommand;
        }
        rdc = checkredirection(pip[i]);
        numargs = 0;

        // parse the chosen block of pipe and parse it into multiple arguments
        char* storedstr[strlen(pip[i])];
        char* dupedpip = strdup(pip[i]);
        char** parsedcmd = parsecmd(dupedpip, storedstr, &numargs);

        // if redirection character detected
        if (!(strcmp(rdc[1], ">"))){
            // if no arguments before redirection character
            if (!(strcmp(rdc[0], ""))){
                return MissingCommand;
            }
            // if the the variable after '$' is invalid, return error message
            if (handlespecialcmd(parsedcmd, numargs)){
                return InvalidVarialbeName;
            }
            // if more than 16 arguments
            if (numargs > MaximunArguments){
                return TooManyArguements;
                // if redirection is not at the end
            } else if (i != *numpipe-1){
                return MislocatedRed;
                // if no arguments after the redirection character
            } else if (!(strcmp(rdc[2], ""))){
                return NoOutputFile;
            }
            mrdc = checkredirection(rdc[2]);
            // multiple redirections
            if ((!strcmp(mrdc[1], ">")) && (!(strcmp(mrdc[0], "")))){
                return NoOutputFile;
            }
            statreturn = stat(rdc[2], &stats);
            if (statreturn != -1){
                // check for writing permission
                if (!(stats.st_mode & S_IWGRP)){
                    return CannotOpen;
                }
            }
        } else {
            // if the the variable after '$' is invalid, return error message
            if (handlespecialcmd(parsedcmd, numargs)){
                return InvalidVarialbeName;
            }
            // if more than 16 arguments
            if (numargs > MaximunArguments){
                return TooManyArguements;
            }
        }
    }
    return 0;
}

char** checkredirection(char* cmd) { // check if redirection character exists
    // return an array of string separated by redirection character
    unsigned long length = strlen(cmd);
    char** rdc = malloc(length+3);
    for (unsigned long i = 0; i < length; i++) {
        if (cmd[i] == '>'){
            rdc[0] = malloc(i+1);
            strncpy(rdc[0], cmd, i);
            rdc[0] = removetrailingspaces(rdc[0], i);
            rdc[1] = malloc(2);
            strncpy(rdc[1], cmd+i, 1);
            rdc[2] = malloc(length-i);
            strncpy(rdc[2], cmd+i+1, length-i-1);
            rdc[2] = removeleadingspaces(rdc[2]);
            rdc[1][-1] = '\0';
            rdc[2][-1] = '\0';
            return rdc;
        }
    }
    rdc[1] = "";
    return rdc;
}

char** checkpipe(char* cmd, int* numpipe){ // check if pipe line exists
    // return am array of string separated by pipe line
    unsigned long length = strlen(cmd);
    char** pipe = malloc(length+4);
    int count = 0;
    unsigned long sum = 0;
    for (unsigned long i = 0; i < length+1; i++){
        if ((cmd[i] == '|') || (cmd[i] == '\0')){
            for (int j = 0; j < count; j++){
                sum += (strlen(pipe[j])+1);
            }
            pipe[count] = malloc(i+1-sum);
            strncpy(pipe[count], cmd+sum, i-sum);
            count++;
            sum = 0;
        }
    }
    for (int i = 0; i < count; i++) {
        pipe[i] = removetrailingspaces(pipe[i], strlen(pipe[i]));
        pipe[i] = removeleadingspaces(pipe[i]);
    }
    *numpipe = count;
    return pipe;
}

char* removetrailingspaces(char* str, unsigned long length){
    // Remove trailing spaces
    for (unsigned long j = 0; j < length; j++){
        if (str[length-j-1] != ' '){
            str[length-j] = '\0';
            break;
        }
    }
    return str;
}

char* removeleadingspaces(char* cmd){
    // Remove leading spaces
    unsigned long numspaces = 0;
    unsigned long length = strlen(cmd);
    for (unsigned long i = 0; i < length+1; i++){
        if (cmd[i] != ' '){
            numspaces = i;
            break;
        }
    }
    cmd += numspaces;
    return cmd;
}

int mysyscall(char *cmd, int* message, int* numpipe)
{
    char** rdc;
    char** pip;
    int curpipe;
    pid_t pid;

    // parsing of the pipe sign from the command line
    pip = checkpipe(cmd, numpipe);
    curpipe = *numpipe;
    int* fdarray[*numpipe-1];
    // parsing of the output redirection from the command line
    rdc = checkredirection(pip[*numpipe-1]);
    // Check if there is any parsing error. If any, return the error value to main func.
    if (handleparsingerror(pip, numpipe)) return handleparsingerror(pip, numpipe);

    // execute cmd with no pipe
    if (*numpipe == 1){
        /* check if the cmd is special one, which doesn't need to fork
         * if it is the a special one, return the return value to main func
         */
        int whethernonfork;
        whethernonfork = nonforkfunc(cmd);
        if (whethernonfork){
            return (whethernonfork -1);
        }
        // run the cmd with no pipe
        return (onepipe(pip, rdc));
    } else {
        pid = fork();
        if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            message[*numpipe-1] = status/ExitStatusDevision;
        } else {
            multpipe(pip, *numpipe, curpipe, rdc, message, fdarray);
        }
    }
    free(pip);
    free(rdc);
    return 0;
}

void printerrormessage(int retval){
    // print error message base on the return value
    if (retval == MissingCommand){
        fprintf(stderr, "Error: missing command\n");
    } else if (retval == TooManyArguements){
        fprintf(stderr, "Error: too many process arguments\n");
    } else if (retval == MislocatedRed){
        fprintf(stderr, "Error: mislocated output redirection\n");
    } else if (retval == NoOutputFile){
        fprintf(stderr, "Error: no output file\n");
    } else if (retval == CannotOpen){
        fprintf(stderr, "Error: cannot open output file\n");
    } else if (retval == InvalidVarialbeName){
        fprintf(stderr, "Error: invalid variable name\n");
    }

}

int main(void)
{
    char cmd[CMDLINE_MAX];

    while (1) {
        char *nl;
        int retval;
        int numpipe; //define the number of pipes
        //malloc space for return message
        int* message = malloc(sizeof(int)*4);

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO)) {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';
        //duplicate a cmd so that the original one will never be modified
        char* dupcmd = strdup(cmd);
        //remove leading and trailing spaces of the duplicated cmd
        dupcmd = removeleadingspaces(dupcmd);
        removetrailingspaces(dupcmd, strlen(dupcmd));

        //re-print prompt if user typed in nothing or just spaces
        if(!strcmp(dupcmd,"")) continue;

        //exit the sshell if the cmd is exit
        if (!strcmp(dupcmd, "exit")) {
            //QUESTION!
            fprintf(stderr, "Bye...\n");
            fprintf(stderr, "+ completed 'exit' [0]\n");
            break;
        }
        // run the cmd by the function defined myself, and get the error value
        retval= mysyscall(dupcmd, message, &numpipe);
        // print error message(if any) base on the return value
        if (retval>=MissingCommand && retval <= InvalidVarialbeName) {
            printerrormessage(retval);
            continue;
        }
        // print complete message base on the number of pipes
        if (numpipe == 1){
            fprintf(stderr, "+ completed '%s' [%d]\n",cmd, retval);
        }else{
            fprintf(stderr, "+ completed '%s' ",cmd);
            for (int i = 0; i < numpipe; ++i) {
                fprintf(stderr, "[%d]", message[i]);
            }
            fprintf(stderr, "\n");
        }
        free(message);
    }
    return EXIT_SUCCESS;
}