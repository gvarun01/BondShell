#include "parser.h"

void parser(char *command_input)
{
    char *command = (char *)malloc((strlen(command_input) + 1) * sizeof(char));
    if (command == NULL)
    {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    strcpy(command, command_input);

    if (strstr(command, "log") == NULL)
    {
        addCommand(command);
    }

    trimstr(command);

    char *x;
    char *cmd = strtok_r(command, ";", &x);
    while (cmd != NULL)
    {
        trimstr(cmd);

        exitCheck(cmd);

        if (cmd[strlen(cmd) - 1] == '|')
        {
            handleError("Invalid Use of Pipes");
            return;
        }

        int in = dup(STDIN_FILENO);
        int out = dup(STDOUT_FILENO);

        char *cmd_copy = (char *)malloc((strlen(cmd) + 1) * sizeof(char));
        if (cmd_copy == NULL)
        {
            handleError("Error allocating memory");
            return;
        }
        strcpy(cmd_copy, cmd);

        char *p;
        char *pipe_cmd = strtok_r(cmd_copy, "|", &p);
        int num_cmds = 0;
        char *commands_in_pipe[MAX_PIPES];

        while (pipe_cmd != NULL)
        {
            commands_in_pipe[num_cmds] = (char *)malloc((strlen(pipe_cmd) + 1) * sizeof(char));
            strcpy(commands_in_pipe[num_cmds], pipe_cmd);
            num_cmds++;
            pipe_cmd = strtok_r(NULL, "|", &p);
        }

        free(cmd_copy);

        if (num_cmds < 1)
        {
            handleError("Invalid Command");
            return;
        }
        else if (num_cmds == 1)
        {
            executeCommand(cmd);
        }
        else
        {
            if (strstr(cmd, "&") != NULL)
            {
                if (cmd[strlen(cmd) - 1] != '&')
                {
                    handleError("Syntax not Correct");
                    return;
                }
                // return;
            }

            // if (num_cmds > MAX_PIPES)
            // {
            //     handleError("Too many pipes");
            //     return;
            // }

            // int pipefd[num_cmds - 1][2];

            // for (int i = 0; i < num_cmds - 1; i++)
            // {
            //    if(i < num_cmds - 1){
            //     if (pipe(pipefd[i]) == -1)
            //     {
            //         handleError("Pipe Error");
            //         return;
            //     }
            //    }
            //    else{
            //         dup2(out, STDOUT_FILENO);
            //    }
            //    char *temp = (char*)malloc(MAX_COMMAND);
            //    strcpy(temp,commands_in_pipe[i]);

            //     if (i > 0)
            //                 dup2(pipefd[i - 1][0], STDIN_FILENO);

            //     executeCommand(temp);

            //     close(pipefd[i-1][0]);
            //     close(pipefd[i][1]);

            if (num_cmds > MAX_PIPES)
            {
                handleError("Too many pipes");
                return;
            }
            int pipefd[num_cmds - 1][2];
            for (int i = 0; i < num_cmds; i++)
            {
                // need to handle piping here and then execute the command
                char *temp = (char *)malloc((strlen(commands_in_pipe[i]) + 1) * sizeof(char));
                strcpy(temp, commands_in_pipe[i]);

                if (i < num_cmds - 1)
                {
                    if (pipe(pipefd[i]) == -1)
                    {
                        handleError("Pipe Error");
                        return;
                    }

                    dup2(pipefd[i][1], STDOUT_FILENO);
                }
                else
                {
                    dup2(out, STDOUT_FILENO);
                }

                if (i != 0)
                {
                    dup2(pipefd[i - 1][0], STDIN_FILENO);
                }

                // printf("Executing %s\n", temp);
                executeCommand(temp);

                close(pipefd[i-1][0]);
                close(pipefd[i][1]);
            }

            dup2(in, STDIN_FILENO);
            dup2(out, STDOUT_FILENO);

        

        // for (int i = 0; i < num_cmds; i++)
        // {
        //     pid_t pid = fork();
        //     if (pid == -1)
        //     {
        //         handleError("Fork Failed");
        //         return;
        //     }
        //     else if (pid == 0)
        //     {
        //         if (i != 0)
        //         {
        //             dup2(pipefd[i - 1][0], STDIN_FILENO);
        //         }
        //         else
        //         {
        //             dup2(in, STDIN_FILENO);
        //         }

        //         if (i != num_cmds - 1)
        //         {
        //             dup2(pipefd[i][1], STDOUT_FILENO);
        //         }
        //         else
        //         {
        //             dup2(out, STDOUT_FILENO);
        //         }

        //         for (int j = 0; j < num_cmds - 1; j++)
        //         {
        //             close(pipefd[j][0]);
        //             close(pipefd[j][1]);
        //         }
        //         executeCommand(commands_in_pipe[i]);
        //         exit(EXIT_FAILURE);
        //     }
        // }

        // for (int i = 0; i < num_cmds - 1; i++)
        // {
        //     close(pipefd[i][0]);
        //     close(pipefd[i][1]);
        // }

        // for (int i = 0; i < num_cmds; i++)
        // {
        //     wait(NULL);
        // }

        // dup2(in, STDIN_FILENO);
        // dup2(out, STDOUT_FILENO);
    }
    for (int i = 0; i < num_cmds; i++)
    {
        free(commands_in_pipe[i]);
    }
    cmd = strtok_r(NULL, ";", &x);
}
free(command);
}