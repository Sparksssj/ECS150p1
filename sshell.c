#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

//declearations
int mysyscall(char *inputcmd);
char** parsecmd(char *cmd, char** storedstr);
bool checkmultipleargs(char *cmd);
int excenoarg(char* cmd);
char* removeleadingspaces(char* cmd);
#define CMDLINE_MAX 512



int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                int retval;

                /* Print prompt */
                printf("sshell$ ");
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
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Regular command */

                retval = mysyscall(cmd);
                fprintf(stdout, "Return status value for '%s': %d\n",
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

        // set the arguments
        char *args[] = {parsedcmd[0], parsedcmd[1], NULL};

        // TO BE packed into a funtion
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            execvp(cmd, args);
            perror("execvp");
            exit(1);
        } else if (pid > 0){
            int status;
            waitpid(pid, &status, 0);
            return status;
        } else{
            return 1;
        }

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

bool checkmultipleargs(char *cmd){ // check if there are multiple areguments

    char duplicatedcmd[strlen(cmd)];
    strcpy(duplicatedcmd, cmd);

    char Delimiter[] = " ";

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
    pid_t pid;
    // set the default args
    char *args[] = {cmd, NULL, NULL};


    // TO BE packed
    pid = fork();
    if (pid == 0) {
        execvp(cmd, args);
        perror("execvp");
        exit(1);
    } else if (pid > 0){
        int status;
        waitpid(pid, &status, 0);
        return status;
    } else{
        return 1;
    }
}

char* removeleadingspaces(char* cmd){ // remvoe the leading spaces of user input
    //https://www.geeksforgeeks.org/c-program-to-trim-leading-white-spaces-from-string/
    //TO BE MODIFIED
    static char removedstr[99];
    int count = 0, j, k;

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