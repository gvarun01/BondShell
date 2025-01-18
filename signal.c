#include "signal.h"

void ping(char *subcom)
{
    char *cmd = strtok(subcom, " \t\n");
    char *pid_str = strtok(NULL, " \t\n");
    char *sig_str = strtok(NULL, " \t\n");

    if (pid_str == NULL || sig_str == NULL)
    {
        handleError("Usage: ping <pid> <signal_number>");
        return;
    }

    pid_t pid = atoi(pid_str);
    int sig = atoi(sig_str) % 32;

    if (kill(pid, sig) == -1)
    {
        if (errno == ESRCH)
        {
            handleError("No such process found");
        }
        else
        {
            perror("kill");
        }
    }
    else
    {
        printf("Sent signal %d to process with pid %d\n", sig, pid);
    }
}

void handleCtrlC(int sig)
{
    if (fgPid != -1)
    {
        if (kill(fgPid, SIGINT) == -1)
        {
            perror("kill");
        }
        else
        {
            printf("\nSent SIGINT to process with pid %d\n", fgPid);
        }
        fgPid = -1;
    }
}

void killAllProcesses()
{
    if (fgPid != -1)
    {
        if (kill(fgPid, SIGKILL) == -1)
        {
            if (errno != ESRCH)
            {
                perror("kill");
            }
        }
        fgPid = -1;
    }
     
    for (int i = 0; i < bgcount; i++)
    {
        if (kill(bgs[i].id, SIGKILL) == -1)
        {
            if (errno != ESRCH)
            {
                perror("kill");
            }
        }
    }
}

void handleCtrlZ(int sig) {


    if (fgPid != -1) {
        if (kill(fgPid, SIGTSTP) == -1) {
            perror("kill");
        } else {

            printf("\nSent SIGTSTP to process with pid %d\n", fgPid);
            setpgid(fgPid, fgPid);
            bgs[bgcount].id = fgPid;
            strcpy(bgs[bgcount].comm, fgCommand);
            bgcount++;
        }
        fgPid = -1;
    }

}
