#include "fgbg.h"

void handleFg(int pid) {
    // printf("pid received: %d\n", pid);
    if (pid == -1) {
        handleError("No such process found");
        return;
    }

    bool found = false;
    int i;
    for (i = 0; i < bgcount; i++) {
        if (bgs[i].id == pid) {
            found = true;
            break;
        }
    }

    if (!found) {
        handleError("No such process found");
        return;
    }

    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);

    int gid = getpgid(pid);

    int rc = tcsetpgrp(0, gid);
    if (rc == -1) {
        perror("tcsetpgrp");
    }
    rc = kill(pid, SIGCONT);
    if (rc == -1) {
        perror("kill");
    }

    fgPid = pid;
    printf("PID: %d brought to foreground\n", pid);
    // strcpy(fgCommand, getname(pid));

    // removing from bg list
    for(int j = i; j < bgcount - 1; j++) {
        bgs[j].id = bgs[j + 1].id;
        strcpy(bgs[j].comm, bgs[j + 1].comm);
    }
    bgcount--;

    // waiting for the process to finish
    int status;
    rc = waitpid(pid, &status, WUNTRACED);
    if (rc == -1) {
        perror("waitpid");
    }
    fgPid = -1;

    // setting terminal back to shell
    signal(SIGTTOU, SIG_IGN);
    int terminalid = getpgid(0);
    rc = tcsetpgrp(0, terminalid);
    if (rc == -1) {
        perror("tcsetpgrp");
    }

    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

void handleBg(int pid){
    if(pid == -1){
        handleError("No such process found");
        return;
    }

    // do not need to handle SIGTTOU and SIGTTIN here
    
    if(kill(pid, SIGCONT) == -1){
        handleError("Process can not be continued");
    }
}