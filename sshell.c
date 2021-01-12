#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>


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
    cmd = removeleadingspaces(inputcmd);
    bool multargs = checkmultipleargs(cmd);

    if (multargs){

        unsigned long init_length = strlen(cmd);
        char* storedstr[init_length];

        char** parsedcmd;
        parsedcmd = parsecmd(cmd, storedstr);


        pid_t pid;
        char *args[] = {parsedcmd[0], parsedcmd[1], NULL};

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
        return (excenoarg(cmd));
    }


}


char** parsecmd(char *cmd, char** storedstr){
//https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
    char* str = cmd;
    char delim[] = " ";

    char *ptr = strtok(str, delim);
    int i = 0;

    while (ptr != NULL)
    {
        storedstr[i] = ptr;
        ptr = strtok(NULL, delim);
        i++;
    }
    return storedstr;
}

bool checkmultipleargs(char *cmd){
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
    if (i == 1) return false;
    else return true;
}

int excenoarg(char* cmd){
    pid_t pid;
    char *args[] = {cmd, NULL, NULL};
    char Delimiter[] = " ";

    char *ptr = strtok(cmd, Delimiter);


    pid = fork();
    if (pid == 0) {
        execvp(ptr, args);
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

char* removeleadingspaces(char* cmd){
    //https://www.geeksforgeeks.org/c-program-to-trim-leading-white-spaces-from-string/
    static char removedstr[99];
    int count = 0, j, k;

    // Iterate String until last
    // leading space character
    while (cmd[count] == ' ') {
        count++;
    }

    // Putting string into another
    // string variable after
    // removing leading white spaces
    for (j = count, k = 0;
         cmd[j] != '\0'; j++, k++) {
        removedstr[k] = cmd[j];
    }
    removedstr[k] = '\0';

    return removedstr;
}