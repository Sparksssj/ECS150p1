#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


int mysyscall(char *cmd);
char *parsecmd(char *cmd);

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

int mysyscall(char *cmd)
{

    pid_t pid;
    char *args[] = {cmd, NULL, NULL};
    pid = fork();

    cmd = parsecmd(cmd);

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

char* parsecmd(char *cmd){

    char* str = cmd;
    char delim[] = " ";

    char *ptr = strtok(str, delim);

    while(ptr != NULL)
    {
        printf("'%s'\n", ptr);
        ptr = strtok(NULL, delim);
    }
    printf("\n");

    return cmd;
}