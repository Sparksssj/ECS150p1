#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>


//declarations
int mysyscall(char *inputcmd);
char** parsecmd(char *cmd, char** storedstr);
bool checkmultipleargs(char *cmd);
int excenoarg(char* cmd);
char* removeleadingspaces(char* cmd);
int forkandexce(char* cmd, char** args);
int changedir(char** parsedcmd);
#define CMDLINE_MAX 512



int main(void)
{
        char cmd[CMDLINE_MAX];

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

                retval = mysyscall(cmd);
                fprintf(stdout, "+ completed '%s' [%d]\n",
                        cmd, retval);
        }

        return EXIT_SUCCESS;
}

int mysyscall(char *inputcmd)
{

    char* cmd;
    //remove the leading white spaces
    cmd = removeleadingspaces(inputcmd);

    //check whether there is any argument
    bool multargs = checkmultipleargs(cmd);

    if (multargs){ // if there are some arguments


        // create an array of char* to store parsed string
        unsigned long init_length = strlen(cmd);
        char* storedstr[init_length];
        char** parsedcmd;

        // store the parsed cmd in the array
        parsedcmd = parsecmd(cmd, storedstr);

        if (!strcmp(parsedcmd[0], "cd")){
            return (changedir(parsedcmd));
        }

        // set the arguments
        char *args[] = {parsedcmd[0], parsedcmd[1], NULL};

        return (forkandexce(cmd,args));

    } else{
        // execute 1 argument program
        return (excenoarg(cmd));
    }


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

int excenoarg(char* cmd){ // execute the no argument command

    // set the default args
    char *args[] = {cmd, NULL, NULL};

    //TODO
    if (!strcmp(cmd, "pwd")){
        printf("%s\n", get_current_dir_name());
        return 0;
    } else{
        return (forkandexce(cmd, args));
    }
}

char* removeleadingspaces(char* cmd){ // remove the leading spaces of user input
    //https://www.geeksforgeeks.org/c-program-to-trim-leading-white-spaces-from-string/
    //TO BE MODIFIED
    static char removedstr[99];
    int count = 0;
    int j, k;

    // Iterate String until last
    // leading space character
    while (cmd[count] == ' ') {
        count++;
    }

    for (j = count, k = 0;
         cmd[j] != '\0'; j++, k++) {
        removedstr[k] = cmd[j];
    }
    removedstr[k] = '\0';

    return removedstr;
}

int forkandexce(char* cmd, char** args){
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        execvp(cmd, args);
        fprintf(stderr, "command not found\n");
        exit(1);
    } else if (pid > 0){
        int status;
        waitpid(pid, &status, 0);
        return status;
    } else{
        return 1;
    }
}

int changedir(char** parsedcmd){

    int chdirresult = chdir(parsedcmd[1]);

    if (chdirresult == -1){
        fprintf(stderr, "cannot cd into directory\n");
        return 1;
    }

    return 0;


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
}