#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>


//declarations
int mysyscall(char *inputcmd, int* message);
char** parsecmd(char *cmd, char** storedstr);
bool checkmultipleargs(char *cmd);
void excenoarg(char* cmd, char** rrdc, bool last);
char* removeleadingspaces(char* cmd);
void forkandexce(char* cmd, char** args, char** rrdc, bool last);
int changedir(char** parsedcmd);
char** checkredirection(char* cmd);
void redirection(char* filename);
char** checkpipe(char* cmd, int* numpipe);
char* removetrailingspaces(char* str, unsigned long length);
void choosenumpipe(char** pipe, int numpipe, int curpipe, char** rdc, int* message, int fd[]);
void onepipe(char** pipe, char** rdc, int* message);
void twopipe(char** pipe, char** rdc);
void threepipe(char** pipe, char** rdc, int* message);
void fourpipe(char** pipe, char** rdc);
void handlearguments(char* cmd, char** rdc, bool last);
#define CMDLINE_MAX 512



int main(void)
{
    char cmd[CMDLINE_MAX];
    int* message = malloc(sizeof(int)*4);

    while (1) {
        char *nl;
        int retval;

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

        /* Builtin command */
        if (!strcmp(cmd, "exit")) {
            //QUESTION!
            fprintf(stderr, "Bye...\n");
            fprintf(stdout, "Return status value for '%s': %d\n",
                    cmd, 0);
            break;
        }

        /* Regular command */

        int numpipe = mysyscall(cmd, message);
        switch(numpipe) {
            case 1:
                fprintf(stdout, "+ completed '%s' [%d]\n",
                        cmd, message[0]);
                break;
            case 2:
                fprintf(stdout, "+ completed '%s' [%d][%d]\n",
                        cmd, message[0], message[1]);
                break;
            case 3:
                fprintf(stdout, "+ completed '%s' [%d][%d][%d]\n",
                        cmd, message[0], message[1], message[2]);
                break;
            case 4:
                fprintf(stdout, "+ completed '%s' [%d][%d][%d][%d]\n",
                        cmd, message[0], message[1], message[2], message[3]);
                break;
            default:
                break;
        }

    }

    return EXIT_SUCCESS;
}

int mysyscall(char *inputcmd, int* message)
{
    char* cmd;
    char** rdc;
    char** pip;
    int numpipe;
    int curpipe;
    int fd[2];
    pid_t pid;
    //remove the leading white spaces
    cmd = removeleadingspaces(inputcmd);

    //parsing of the pipe sign from the command line
    pip = checkpipe(cmd, &numpipe);
    curpipe = numpipe;

    //parsing of the output redirection from the command line
    rdc = checkredirection(pip[numpipe-1]);

    pid = fork();
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        message[numpipe-1] = status;
    } else {
        pipe(fd);
        choosenumpipe(pip, numpipe, curpipe, rdc, message, fd);
    }

    return numpipe;
    //check whether there is any argument
    //bool multargs = checkmultipleargs(cmd);

    //if (multargs){ // if there are some arguments


    // create an array of char* to store parsed string
    //unsigned long init_length = strlen(cmd);
    //char* storedstr[init_length];
    //char** parsedcmd;

    // store the parsed cmd in the array
    //parsedcmd = parsecmd(cmd, storedstr);

    //if (!strcmp(parsedcmd[0], "cd")){
    //    return (changedir(parsedcmd));
    //}

    // set the arguments
    //char *args[] = {parsedcmd[0], parsedcmd[1], NULL};

    //return (forkandexce(cmd,args,rdc));

    //} else{
    // execute 1 argument program
    //return (excenoarg(cmd,rdc));
    //}


}


char** parsecmd(char *cmd, char** storedstr){ // parse the cmd by white spaces

//https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split

    char* str = cmd;
    char Deliminator[] = " ";

    char *ptr = strtok(str, Deliminator);
    int i = 0;

    //store every separated command into the array
    while (ptr != NULL)
    {
        storedstr[i] = ptr;
        ptr = strtok(NULL, Deliminator);
        i++;
    }
    return storedstr;
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

    //TODO
    if (!strcmp(cmd, "pwd")){
        printf("%s\n", get_current_dir_name());
        return;
    } else{
        forkandexce(cmd, args, rrdc, last);
        return;
    }
}

char* removeleadingspaces(char* cmd){
    unsigned long numspaces;
    unsigned long length = strlen(cmd);
    for (unsigned long i = 0; i < length; i++){
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
    fprintf(stderr,"%s\n", cmd);
    execvp(cmd, args);
    fprintf(stderr, "command not found\n");
    exit(1);
}

int changedir(char** parsedcmd) {

    int chdirresult = chdir(parsedcmd[1]);

    if (chdirresult == -1) {
        fprintf(stderr, "cannot cd into directory\n");
        return 1;
    }

    return 0;
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

void choosenumpipe(char** pip, int numpipe, int curpipe, char** rdc, int message[], int fd[]) {
    if (numpipe == 1){
        handlearguments(pip[0], rdc, true);
        return;
    }
    pid_t pid;
    pid = fork();
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        fprintf(stderr,"%d\n", curpipe);
        message[curpipe-2] = status;
        if (curpipe == numpipe) {
            fprintf(stderr,"d\n");
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            handlearguments(pip[numpipe-1], rdc, true);
        }
        if  (curpipe < numpipe){
            fprintf(stderr,"c\n");
            fprintf(stderr,"%s\n", pip[curpipe-1]);
            dup2(fd[0], STDIN_FILENO);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
            handlearguments(pip[curpipe-1], rdc, false);
        }
    } else if (pid == 0) {
        if (curpipe > 2){
            fprintf(stderr,"b\n");
            choosenumpipe(pip, numpipe, curpipe-1, rdc, message, fd);
        } else {
            fprintf(stderr,"a\n");
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            handlearguments(pip[0], rdc, false);
        }
    } else {
        message[numpipe-curpipe] = -1;
    }
}


void onepipe(char** pip, char** rdc, int* message){
    pid_t pid;
    pid = fork();
    if (pid > 0){
        int status;
        waitpid(pid, &status, 0);
        message[0] = status;
    } else if (pid == 0){
        handlearguments(pip[0], rdc, true);
    } else {
        message[0] = -1;
    }
}

void twopipe(char** pip, char** rdc){

}

void threepipe(char** pip, char** rdc, int* message){
    pid_t pid;
    pid = fork();
    if (pid > 0){
        int status;
        waitpid(pid, &status, 0);
        message[0] = status;
    } else if (pid == 0){
        int fd[2];
        pipe(fd);
        pid = fork();
        if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            message[1] = status;
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            handlearguments(pip[2], rdc, true);
        } else if (pid == 0){
            pid = fork();
            if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                message[2] = status;
                dup2(fd[0], STDIN_FILENO);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                handlearguments(pip[1], rdc, false);
            } else if (pid == 0){
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                handlearguments(pip[0], rdc, false);
            } else {
                message[2] = -1;
            }
        } else {
            message[1] = -1;
        }
    } else {
        message[0] = -1;
    }
}

void fourpipe(char** pip, char** rdc){

}

void handlearguments(char* cmd, char** rdc, bool last){
    bool multargs = checkmultipleargs(cmd);

    if (multargs){ // if there are some arguments


        // create an array of char* to store parsed string
        unsigned long init_length = strlen(cmd);
        char* storedstr[init_length];
        char** parsedcmd;

        // store the parsed cmd in the array
        parsedcmd = parsecmd(cmd, storedstr);

        if (!strcmp(parsedcmd[0], "cd")){
            changedir(parsedcmd);
        }

        // set the arguments
        char *args[] = {parsedcmd[0], parsedcmd[1], NULL};

        forkandexce(cmd,args,rdc,last);

    } else{
        // execute 1 argument program
        excenoarg(cmd,rdc,last);
    }
}
//    pid_t pid;
//    pid = fork();
//    if (pid == 0) {
//        int chdirresult;
//        chdirresult = chdir(parsedcmd[1]);
//        if (chdirresult == -1){
//            perror("Error: ");
//            exit(1);
//        } else{
//            exit(0);
//        }
//
//    } else if (pid > 0){
//        int status;
//        waitpid(pid, &status, 0);
//        return status;
//    } else{
//        return 1;
//    }
