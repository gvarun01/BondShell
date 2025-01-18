#include "bg.h"

void bg(char *subcom_input)
{
    char *subcom = (char *)malloc(MAX_COMMAND * sizeof(char));
    strcpy(subcom, subcom_input);
    char *temp = handleIO(subcom);
    if (temp == NULL)
    {
        return;
    }
    strcpy(subcom, temp);
    free(temp);
    // printf("the command going to execute is %s\n", subcom);
    
    int child = fork();

    if (child == -1)
    {
        handleError("fork");
        return;
    }
    else if (child == 0)
    { // child
        trimstr(subcom);
        setpgrp();
        systemComamnd(subcom);
    }
    else
    { // parent
        // fprintf(stdout_copy, "%d\n", child);
        dprintf(stdout_copy_fd, "%d\n", child);
        bgs[bgcount].id = child;
        strcpy(bgs[bgcount].comm, subcom);
        bgcount++;
        
    }
    resetIO();
    free(subcom);
}   

void delete_bg(int pid)
{
    int i;
    for (i = 0; i < bgcount; i++)
    {
        if (bgs[i].id == pid)
        {
            int j;
            for (j = i; j < bgcount - 1; j++)
            {
                bgs[j].id = bgs[j + 1].id;
                strcpy(bgs[j].comm, bgs[j + 1].comm);
            }
            bgcount--;
            break;
        }
    }
}

char *getname(int pid)
{
    int i;
    for (i = 0; i < bgcount; i++)
    {
        if (bgs[i].id == pid)
        {
            return bgs[i].comm;
        }
    }
    return NULL;
}

void checkbg()
{
    int status;
    int pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        char *name = getname(pid);
        if (WIFEXITED(status))
        {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0)
            {
                dprintf(stdout_copy_fd, COLOR_MAGENTA"%s" COLOR_RESET " exited normally (%d)\n",name, pid);
            }
            else
            {
                dprintf(stdout_copy_fd, COLOR_MAGENTA "%s" COLOR_RESET " exited abnormally with status %d (%d)\n", name, exit_status,pid);
            }
            delete_bg(pid);
        }
        else if (WIFSIGNALED(status))
        {
            dprintf(stdout_copy_fd, COLOR_MAGENTA "%s" COLOR_RESET " exited due to signal %d (%d)\n", name, WTERMSIG(status),pid);
            delete_bg(pid);
        }
        else{
            // dprintf(stdout_copy_fd, "bond is gr0kdpisn\n");
            // teri maa ki chut
        }
    }
}
