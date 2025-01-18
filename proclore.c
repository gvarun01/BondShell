#include "proclore.h"

void proclore(int pid)
{
    if (pid == 0)
    {
        pid = getpid();
    }

    char state;
    int vm_size = 0;
    int tgid;
    char fb = ' ';

    char statusPath[MAX_PATH] = "";
    sprintf(statusPath, "/proc/%d/status", pid);

    FILE *statusFile = fopen(statusPath, "r");
    if (statusFile == NULL)
    {
        handleError("fopen failed in proclore");
        return;
    }

    char line[MAX_PATH] = "";
    while (fgets(line, MAX_PATH, statusFile))
    {
        if (strncmp(line, "State:", 6) == 0)
        {
            sscanf(line + 7, "%c", &state);
        }
        else if (strncmp(line, "VmSize:", 7) == 0)
        {
            sscanf(line + 8, "%d", &vm_size);
        }
        else if (strncmp(line, "Tgid:", 5) == 0)
        {
            sscanf(line + 6, "%d", &tgid);
        }
    }

    fclose(statusFile);

    pid_t pgid = getpgid(pid);
    pid_t fg_pgid = tcgetpgrp(STDIN_FILENO);

    if (pid == fg_pgid) {
        fb = '+';
    }

    char exePath[MAX_PATH] = "";
    sprintf(exePath, "/proc/%d/exe", pid);

    char line2[MAX_PATH] = "";
    int rc = readlink(exePath, line2, MAX_PATH);
    if (rc == -1)
    {
        handleError("readlink");
        return;
    }
    line2[rc] = '\0';

    char *newPath = relativePath(line2);

    printf("Process ID: %d\n", pid);
    printf("State: %c%c\n", state, fb);
    printf("Process Group ID: %d\n", tgid);
    printf("Virtual Memory Size: %d\n", vm_size);
    printf("Executable Path: %s\n", newPath);
}