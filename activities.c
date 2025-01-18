#include "activities.h"

int compareProcessID(const void *a, const void *b) {
    return ((struct bgprocess *)a)->id - ((struct bgprocess *)b)->id;
}

void activities() {
    char currentState[32];
    qsort(bgs, bgcount, sizeof(struct bgprocess), compareProcessID);

    for (int i = 0; i < bgcount; i++) {
        int pid = bgs[i].id;
        char procPath[256];
        snprintf(procPath, sizeof(procPath), "/proc/%d/stat", pid);

        FILE *fp = fopen(procPath, "r");
        if (fp == NULL) {
            // perror("fopen");
            handleError("fopen failed in activities");
            continue;
        }

        int ppid, pgrp, session, tty_nr, tpgid;
        char comm[256], state;
        fscanf(fp, "%d %s %c %d %d %d %d %d", &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid);
        fclose(fp);

        if (state == 'T' || state == 'D') {
            strcpy(currentState, "Stopped");
        } else {
            strcpy(currentState, "Running");
        }

        printf("%d : %s - %s\n", pid, bgs[i].comm, currentState);
    }
}