#include "syscmd.h"

int systemComamnd(char *command)
{
    char *cmd = strtok(command, " \t\n");
    char *args[MAX_COMMAND];
    int i = 0;

    while (cmd != NULL)
    {
        if (strcmp(cmd, "~") == 0)
        {
            // strcpy(newPath, home);
            args[i] = home;
        }
        else if (strncmp(cmd, "~", 1) == 0)
        {
            char *newPath = (char *)malloc(MAX_PATH);
            strcpy(newPath, home);
            strcat(newPath, cmd + 1);
            args[i] = newPath;
        }
        else if (strcmp(cmd, ".") == 0)
        {
            char *temp = (char *)malloc(MAX_PATH);
            if (getcwd(temp, MAX_PATH) == NULL)
            {
                handleError("getcwd");
                return -1;
            }
            args[i] = temp;
        }
        else
        {
            args[i] = cmd;
        }
        i++;
        cmd = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    int rc = execvp(args[0], args);
    fflush(stdout);
    if (rc == -1)
    {
        handleError("execvp");
        return 1;
    }
    else
    {
        if (WIFEXITED(rc))
        {
            int exit = WEXITSTATUS(rc); // 0 if successful
            if (exit == 0)
                printf("%s executed successfully.\n", input);
            else
                // printf(ERROR_COLOR "%s executed with non-zero exit status: %d\n" DEFAULT_COLOR, input, exit);
                printf(COLOR_ERROR "%s executed with non-zero exit status: %d\n" COLOR_RESET, input, exit);
        }
        else if (WIFSIGNALED(rc))
            printf(COLOR_ERROR "%s terminated by signal.\n" COLOR_RESET, input);
    }
    for(int j = 0; j < i; j++)
    {
        free(args[j]);
    }
    return 0;
}