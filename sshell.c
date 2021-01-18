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
void choosenumpipe(char** pipe, int numpipe, int curpipe, char** rdc, int* message, int* fdarray[]);
void handlearguments(char* cmd, char** rdc, bool last);
#define CMDLINE_MAX 512



int main(void)
{
    char cmd[CMDLINE_MAX];
    int* message = malloc(sizeof(int)*4);

    while (1) {
        char *nl;


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
    int* fdarray[3];
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
        choosenumpipe(pip, numpipe, curpipe, rdc, message, fdarray);
    }

    return numpipe;
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
    unsigned long numspaces = 0;
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
    execvp(cmd, args);
    perror("Error");
    exit(1);
}

int changedir(char** parsedcmd) {

    int chdirresult = chdir(parsedcmd[1]);

    if (chdirresult == -1) {
        perror("Error");
        exit(1);
    }

    exit(0);
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

void choosenumpipe(char** pip, int numpipe, int curpipe, char** rdc, int message[], int* fdarray[]) {
    if (numpipe == 1){
        if (!(strcmp(rdc[1], ">"))){
            handlearguments(rdc[0], rdc, true);
        } else {
            handlearguments(pip[0], rdc, true);
        }
        return;
    }
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
            choosenumpipe(pip, numpipe, curpipe-1, rdc, message, fdarray);
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

        if (!strcmp(parsedcmd[0], "cd")){
            changedir(parsedcmd);
        }

        // set the arguments

        char **args = malloc(sizeof(char*)*(numargs+1));

        for (int i = 0; i < numargs; ++i) {
            args[i] = parsedcmd[i];
        }
        args[-1] = NULL;



        forkandexce(cmd,args,rdc,last);

    } else{
        // execute 1 argument program
        excenoarg(cmd,rdc,last);
    }
}
