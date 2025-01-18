#include "fg.h"

void executeInFG(char *command_input)
{
    char *command = (char *)malloc(MAX_COMMAND);
    strcpy(command, command_input);

    char *temp = handleIO(command);
    if(temp == NULL){
        return;
    }
    strcpy(command, temp);
    free(temp);
    trimstr(command);
    strcpy(fgCommand, command);

    // printf("came here in fg.c file\n");

    exitCheck(command);

    if (strcmp(command, "log") == 0)
    {
        printLog();
        return;
    }

    else if (strcmp(command, "log purge") == 0)
    {
        purgeLog();
        return;
    }

    else if (strncmp(command, "log execute", 11) == 0)
    {
        int index;
        sscanf(command + 12, "%d", &index);
        char *cmd = executeLog(index);
        if (cmd == NULL)
        {
            handleError("log execution failed");
            return;
        }
        else
        {
            cmd[strlen(cmd) - 1] = '\0';
            parser(cmd);
            return;
        }
    }

    if (strncmp(command, "cd", 2) == 0)
    {
        char *str = command + 3;

        char current[MAX_PATH];
        if (getcwd(current, sizeof(current)) == NULL)
        {
            handleError("getcwd failed in cd");
            return;
        }

        if(strcmp(command, "cd") == 0){
            str = NULL;
        }

        if (str == NULL || strlen(str) == 0)
        {
            if (chdir(home) == -1)
            {
                handleError("chdir failed");
                return;
            }
            getcwd(current, sizeof(current));
            return;
        }
        char path[MAX_PATH];
        sscanf(str, "%s", path);
        trimstr(path);

        char *newPath = pathCorrect(path);

        if (strcmp(path, "-") == 0)
        {
            if (strlen(lastDir) == 0)
            {
                handleError("No previous directory");
                return;
            }
            strcpy(newPath, lastDir);
        }

        if (chdir(newPath) == -1)
        {
            handleError("chdir failed");
            return;
        }

        strcpy(lastDir, current);
        if (getcwd(current, sizeof(current)) == NULL)
        {
            handleError("getcwd failed in hop");
            return;
        }
    }

    else if (strncmp(command, "help" , 4) == 0 || strncmp(command, "help bond", 9) == 0 || strncmp(command, "bond help", 9) == 0)
    {
        displayHelp();
    }

    else if (strncmp(command, "iMan", 4) == 0)
    {
        iman(command);
    }

    else if (strncmp(command, "ping", 4) == 0)
    {
        ping(command);
    }

    else if (strncmp(command, "activities", 10) == 0)
    {
        activities();
    }

    else if (strncmp(command, "fg", 2) == 0)
    {
        char *x;
        char *pid_s = strtok_r(command + 3, " \t\n", &x);
        int pid = 0;
        if (pid_s != NULL)
        {
            sscanf(pid_s, "%d", &pid);
        }
        handleFg(pid);
    }

    else if (strncmp(command, "bg", 2) == 0)
    {
        // bg(command);
        char *x;
        char *pid_s = strtok_r(command + 3, " \t\n", &x);
        int pid = 0;
        if (pid_s != NULL)
        {
            sscanf(pid_s, "%d", &pid);
        }
        handleBg(pid);
    }

    else if (strncmp(command, "neonate", 7) == 0)
    {
        neonate(command + 8);
    }

    else if (strncmp(command, "proclore", 8) == 0)
    {
        char *x;
        char *pid_s = strtok_r(command + 9, " \t\n", &x);
        int pid = 0;
        if (pid_s != NULL)
        {
            sscanf(pid_s, "%d", &pid);
        }
        proclore(pid);
    }

    else if (strncmp(command, "reveal", 6) == 0)
    {
        reveal(command + 7);
    }

    else if (strncmp(command, "seek", 4) == 0)
    {
        seek(command + 5);
    }

    else if (strncmp(command, "hop", 3) == 0)
    {
        // printf("hop executing with string : %s\n", command);
        if(strcmp(command, "hop") == 0){
            hop(NULL);
        }
        else{
            hop(command + 4);
        }
        // hop(command + 4);
    }

    else
    {
        char *args[4096];
        char *y = NULL;
        char *token = strtok_r(command, " \t\n", &y);
        int i = 0;
        while (token != NULL)
        {
            args[i] = token;
            i++;
            token = strtok_r(NULL, " \t\n", &y);
        }
        args[i] = NULL;
        int child = fork();
        if (child == 0)
        {
            if(strncmp(args[0], "nano" , 4) != 0 && strncmp(args[0], "vi" , 2) != 0 && strncmp(args[0], "vim" , 3) != 0){
                setpgid(0, 0);
            }

            if (execvp(args[0], args) < 0)
            {
                // printf(COLOR_ERROR "Invalid command.\n" COLOR_RESET);
                handleError("Invalid command or System call failed");
                exit(EXIT_FAILURE);
            }
            fflush(stdout);
        }
        else if(child > 0)
        {
            fgPid = child;
            // signal(SIGTTOU, SIG_IGN);
            // signal(SIGTTIN, SIG_IGN);
            // tcsetpgrp(0, fgPid);
            int status;
            waitpid(child, &status, WUNTRACED);


            // if(WEXITSTATUS(status) != 0){
            //     // printf("Process %d exited normally\n", child);
            //     // delete
            //     delete_bg(child);
            // }
            // tcsetpgrp(STDIN_FILENO, getpgrp());
            // tcsetpgrp(0, getpgid(0));
            // signal(SIGTTOU, SIG_DFL);
            // signal(SIGTTIN, SIG_DFL);

            if(WIFEXITED(status)){
                if(WEXITSTATUS(status) != 0){
                    printf("Process %d exited with status %d\n", child, WEXITSTATUS(status));
                }
            }
            fgPid = -1;
        }
    }

    free(command);
    resetIO();
}

