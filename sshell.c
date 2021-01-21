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
void excenoarg(char* cmd, char** rrdc, bool last);
char* removeleadingspaces(char* cmd);
void forkandexce(char* cmd, char** args, char** rrdc, bool last);
int changedir(char** parsedcmd);
char** checkredirection(char* cmd);
void redirection(char* filename);
char** checkpipe(char* cmd, int* numpipe);
char* removetrailingspaces(char* str, unsigned long length);
void multpipe(char** pipe, int numpipe, int curpipe, char** rdc, int* message, int* fdarray[]);
void handlearguments(char* cmd, char** rdc, bool last);
void handlespecialcmd(char** parsedcmd, int numargs);
int setavariable(char** parsedcmd);
int onepipe(char** pipe, char** rdc);
int nonforkfunc(char* cmd);
int handleparsingerror(char** pip, const int* numpipe);
int countarguments(char* cmd);
#define CMDLINE_MAX 512
char* storedvariable[26];

int main(void)
{
   // char **storedvariable = malloc(sizeof(char*) * 26);
    char cmd[CMDLINE_MAX];
    int* message = malloc(sizeof(int)*4);

    while (1) {
        char *nl;
        int retval;
        int numpipe;

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

        dupcmd = removeleadingspaces(dupcmd);
        removetrailingspaces(dupcmd, strlen(dupcmd));

        if(!strcmp(dupcmd,"")) continue;

        /* Builtin command */
        if (!strcmp(dupcmd, "exit")) {
            //QUESTION!
            fprintf(stderr, "Bye...\n");
            fprintf(stderr, "+ completed 'exit' [0]\n");
            break;
        }

        /* Regular command */



        retval= mysyscall(dupcmd, message, &numpipe);

        if (retval == 3){
            fprintf(stderr, "Error: missing command\n");
            continue;
        } else if (retval == 4){
            fprintf(stderr, "Error: too many process arguments\n");
            continue;
        } else if (retval == 5){
            fprintf(stderr, "Error: mislocated output redirection\n");
            continue;
        } else if (retval == 6){
            fprintf(stderr, "Error: no output file\n");
            continue;
        } else if (retval == 7){
            fprintf(stderr, "Error: cannot open output file\n");
            continue;
        }

        if (numpipe == 1){
            fprintf(stderr, "+ completed '%s' [%d]\n",cmd, retval/256);
        }else{
            fprintf(stderr, "+ completed '%s' ",cmd);
            for (int i = 0; i < numpipe; ++i) {
                fprintf(stderr, "[%d]", message[i]/256);
            }
            fprintf(stderr, "\n");
        }

    }

    return EXIT_SUCCESS;
}

int mysyscall(char *inputcmd, int* message, int* numpipe)
{
    char* cmd;
    char** rdc;
    char** pip;
    int curpipe;
    int* fdarray[3];
    int error;
    pid_t pid;
    //remove the leading white spaces

    cmd = removeleadingspaces(inputcmd);

    //parsing of the pipe sign from the command line
    pip = checkpipe(cmd, numpipe);
    curpipe = *numpipe;

    //parsing of the output redirection from the command line
    rdc = checkredirection(pip[*numpipe-1]);

    error = handleparsingerror(pip, numpipe);
    if (error){
        return error;
    }

    if (*numpipe == 1){
        int whethernonfork;

        whethernonfork = nonforkfunc(cmd);
        if (whethernonfork){
            return (whethernonfork -1);
        }
        return (onepipe(pip, rdc));
    } else {
        pid = fork();
        if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            message[*numpipe-1] = status;
        } else {
            multpipe(pip, *numpipe, curpipe, rdc, message, fdarray);
        }
    }
    return 0;
}


char** parsecmd(char *cmd, char** storedstr, int* numargs){ // parse the cmd by white spaces

//https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split

    char* str = cmd;
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

int countarguments(char* cmd){
    int numargs = 0;
    char* str = cmd;
    char Deliminator[] = " ";
    char *ptr = strtok(str, Deliminator);
    while (ptr != NULL)
    {
        ptr = strtok(NULL, Deliminator);
        numargs++;
    }
    return numargs;
}

bool checkmultipleargs(char *cmd){ // check if there are multiple arguments

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

void excenoarg(char* cmd, char** rrdc, bool last){ // execute the no argument command

    // set the default args
    char *args[] = {cmd, NULL, NULL};

    forkandexce(cmd, args, rrdc, last);

}

char* removeleadingspaces(char* cmd){
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

void forkandexce(char* cmd, char** args, char** rrdc, bool last){
    if ((!(strcmp(rrdc[1], ">"))) && (last == true)){
        redirection(rrdc[2]);
    }
    execvp(cmd, args);
    perror("Error");
    exit(1);
}

char** checkredirection(char* cmd) {
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

void redirection(char* filename){
    int fd;
    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    dup2(fd, STDOUT_FILENO);
    close(fd);
}

char** checkpipe(char* cmd, int* numpipe){
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
    for (unsigned long j = 0; j < length; j++){
        if (str[length-j-1] != ' '){
            str[length-j] = '\0';
            break;
        }
    }
    return str;
}

void multpipe(char** pip, int numpipe, int curpipe, char** rdc, int message[], int* fdarray[]) {
    int fd[2];
    pipe(fd);
    fdarray[curpipe-2] = fd;
    pid_t pid;
    pid = fork();
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        message[curpipe-2] = status;
        if (curpipe == numpipe) {
            close(fdarray[curpipe-2][1]);
            dup2(fdarray[curpipe-2][0], STDIN_FILENO);
            close(fdarray[curpipe-2][0]);
            if (!(strcmp(rdc[1], ">"))){
                handlearguments(rdc[0], rdc, true);
            } else {
                handlearguments(pip[numpipe-1], rdc, true);
            }
        }
        if  (curpipe < numpipe){
            dup2(fdarray[curpipe-2][0], STDIN_FILENO);
            dup2(fdarray[curpipe-1][1], STDOUT_FILENO);
            close(fdarray[curpipe-2][0]);
            close(fdarray[curpipe-2][1]);
            close(fdarray[curpipe-1][0]);
            close(fdarray[curpipe-1][1]);
            handlearguments(pip[curpipe-1], rdc, false);
        }
    } else if (pid == 0) {
        if (curpipe > 2){
            multpipe(pip, numpipe, curpipe-1, rdc, message, fdarray);
        } else {
            close(fdarray[curpipe-2][0]);
            dup2(fdarray[curpipe-2][1], STDOUT_FILENO);
            close(fdarray[curpipe-2][1]);
            handlearguments(pip[0], rdc, false);
        }
    } else {
        message[numpipe-curpipe] = -1;
    }
}

void handlearguments(char* cmd, char** rdc, bool last){

    int numargs = 0;
    bool multargs = checkmultipleargs(cmd);


    if (multargs){ // if there are some arguments

        // create an array of char* to store parsed string
        unsigned long init_length = strlen(cmd);
        char* storedstr[init_length];
        char** parsedcmd;

        // store the parsed cmd in the array
        parsedcmd = parsecmd(cmd, storedstr, &numargs);

        handlespecialcmd(parsedcmd, numargs);


        // set the arguments
        char **args = malloc(sizeof(char*)*(numargs+1));

        for (int i = 0; i < numargs; ++i) {
            args[i] = parsedcmd[i];
        }
        args[-1] = NULL;

        forkandexce(parsedcmd[0],args,rdc,last);

    } else{
        // execute 1 argument program
        excenoarg(cmd,rdc,last);
    }
}

void handlespecialcmd(char** parsedcmd, int numargs){


    for (int i = 0; i < numargs; ++i) {

        if (parsedcmd[i][0] == '$') {

         //TODO
            if (strlen(parsedcmd[i]) != 2 || parsedcmd[i][1] > 122 || parsedcmd[i][1] < 97){
                fprintf(stderr, "Error: invalid variable name\n");
                exit(1);
            }

            if (storedvariable[parsedcmd[i][1]-97] == 0){
                parsedcmd[i] = "";
            }else{
                parsedcmd[i] = strdup(storedvariable[parsedcmd[i][1]-97]);

            }
        }
    }


}

int setavariable(char** parsedcmd){

    if (strlen(parsedcmd[1]) != 1 || parsedcmd[1][0] > 122 || parsedcmd[1][0] < 97){
        fprintf(stderr, "Error: invalid variable name\n");
        return (1);
    }
    storedvariable[parsedcmd[1][0] - 97] = strdup(parsedcmd[2]);


    return (0);
}

int onepipe(char** pipe, char** rdc){
    pid_t pid;
    pid = fork();

    if (pid > 0){
        int status;
        waitpid(pid, &status, 0);
        return status;
    } else if (pid == 0){
        if (!(strcmp(rdc[1], ">"))){
            handlearguments(rdc[0], rdc, true);
        } else {
            handlearguments(pipe[0], rdc, true);
        }
    } else {
        return (1);
    }
    return 0;
}

int nonforkfunc(char* cmd){
    int numargs = 0;
    bool multargs = checkmultipleargs(cmd);
    unsigned long init_length = strlen(cmd);
    char* storedstr[init_length];
    char** parsedcmd;

    // store the parsed cmd in the array
    parsedcmd = parsecmd(cmd, storedstr, &numargs);

    if (multargs){
        if (!strcmp(parsedcmd[0],"set")){
            return (setavariable(parsedcmd) + 1);
        } else if (!strcmp(parsedcmd[0],"cd")){
            int chdirresult = chdir(parsedcmd[1]);

            if (chdirresult == -1) {
                perror("Error");
                return (2);
            }
            return (1);
        }
    } else{
        if (!strcmp(parsedcmd[0],"pwd")){
            printf("%s\n", get_current_dir_name());
            return (1);
        } else if (!strcmp(parsedcmd[0],"set")){
            fprintf(stderr, "Error: invalid variable name\n");
            return (2);
        }
    }
    return 0;
}

int handleparsingerror(char** pip, const int* numpipe){
    int statreturn;
    struct stat stats;
    char** rdc;
    int numargs;
    for (int i = 0; i < *numpipe; i++){
        if (!(strcmp(pip[i], ""))){
            return 3;
        }
        rdc = checkredirection(pip[i]);
        if (!(strcmp(rdc[1], ">"))){
            if (!(strcmp(rdc[0], ""))){
                return 3;
            }
            numargs = countarguments(rdc[0]);
            if (numargs > 16){
                return 4;
            } else if (i != *numpipe-1){
                return 5;
            } else if (!(strcmp(rdc[2], ""))){
                return 6;
            }
            statreturn = stat(rdc[2], &stats);
            if (statreturn != -1){
                if (!(stats.st_mode & S_IWGRP)){
                    return 7;
                }
            }
        } else {
            numargs = countarguments(pip[i]);
            if (numargs > 16){
                return 4;
            }
        }
    }
    return 0;
}
