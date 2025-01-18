#include "execute.h"

void executeCommand(char *command_got)
{
    char *cmd = (char *)malloc(MAX_COMMAND);
    strcpy(cmd, command_got);
    // printf("came here in execute.c file\n");

    bool is_bg = false;
    if (cmd[strlen(cmd) - 1] == '&')
    {
        is_bg = true;
        cmd[strlen(cmd) - 1] = '\0';
    }

    char *cmd_copy = strdup(cmd);
    char *x1;
    char *cmd1 = strtok_r(cmd, "&", &x1);

    int num_mpercent = -1;
    while (cmd1 != NULL)
    {
        num_mpercent++;
        cmd1 = strtok_r(NULL, "&", &x1);
    }

    if (is_bg)
    {
        num_mpercent++;
    }

    if (num_mpercent == 0)
    {
        trimstr(cmd);

        exitCheck(cmd);

        struct timeval start, end;
        gettimeofday(&start, NULL);
        // printf("1. cmd going to execute is %s\n", cmd);
        executeInFG(cmd);
        // printf("here executed 1\n");
        // printf("here\n");
        gettimeofday(&end, NULL);
        executionTime = (int)((end.tv_sec - start.tv_sec));
        strcpy(executedCommand, cmd);

        if (executionTime > 2)
        {
            if (strlen(promtExec) > 0)
            {
                strcat(promtExec, " | ");
            }
            char *new_executedCommand = getcmd(executedCommand);
            strcat(promtExec, new_executedCommand);
            char buf[100];
            sprintf(buf, " : [%ds]", executionTime);
            strcat(promtExec, buf);
        }
    }
    else
    {
        strcpy(cmd, cmd_copy);
        cmd1 = strtok_r(cmd, "&", &x1);
        for (int i = 0; i < num_mpercent; i++)
        {
            trimstr(cmd1);
            exitCheck(cmd1);

            // printf("1. cmd going to execute in bg is %s\n", cmd1);
            bg(cmd1);
            cmd1 = strtok_r(NULL, "&", &x1);
        }
        if (cmd1 != NULL)
        {
            trimstr(cmd1);
            exitCheck(cmd1);
            
            struct timeval start, end;
            gettimeofday(&start, NULL);
            // printf("2. cmd going to execute in FG is %s\n", cmd1);
            executeInFG(cmd);
            // printf("here executed 2\n");
            gettimeofday(&end, NULL);
            executionTime = (int)((end.tv_sec - start.tv_sec));
            strcpy(executedCommand, cmd1);

            if (executionTime > 2)
            {
                if (strlen(promtExec) > 0)
                {
                    strcat(promtExec, " | ");
                }
                char *new_executedCommand = getcmd(executedCommand);
                strcat(promtExec, new_executedCommand);
                char buf[100];
                sprintf(buf, " : [%ds]", executionTime);
                strcat(promtExec, buf);
            }
        }
    }
}