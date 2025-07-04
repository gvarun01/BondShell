#include "../include/fgbg.h"
#include "../utils/error_handler.h" // For handleError
#include <stdio.h>  // For printf, perror
#include <signal.h> // For kill, SIGCONT, signal, SIGTTOU, SIGTTIN, SIG_DFL, SIG_IGN
#include <unistd.h> // For getpgid, tcsetpgrp, getpgrp
#include <sys/wait.h> // For waitpid, WUNTRACED
#include <string.h> // For strcpy (if getname was used for fgCommand)
#include <errno.h>  // For errno with kill

// Brings a background process to the foreground
void handleFg(pid_t pid_to_fg) { // Changed pid to pid_to_fg for clarity
    if (pid_to_fg <= 0) { // Basic PID validation
        handleError("fg: Invalid PID provided.");
        return;
    }

    int process_index = -1; // Index in the bgs array
    for (int i = 0; i < bgcount; i++) {
        if (bgs[i].id == pid_to_fg) {
            process_index = i;
            break;
        }
    }

    if (process_index == -1) {
        char err_msg[100];
        snprintf(err_msg, sizeof(err_msg), "fg: Job with PID %d not found.", pid_to_fg);
        handleError(err_msg);
        return;
    }

    // Store command name before removing from bgs list for fgCommand
    // Ensure fgCommand is large enough
    strncpy(fgCommand, bgs[process_index].comm, MAX_COMMAND - 1);
    fgCommand[MAX_COMMAND - 1] = '\0';

    // Terminal control dance:
    // 1. Give terminal control to the process group of the job being foregrounded.
    // 2. Send SIGCONT to the process group to resume it if stopped.
    // 3. Wait for the job to complete or stop.
    // 4. Restore terminal control to the shell.

    pid_t process_group_id = getpgid(pid_to_fg);
    if (process_group_id == -1) {
        perror("fg: getpgid failed");
        // This is an issue, might not be able to control the process correctly.
        // Proceeding might be risky, but shell should try.
    }

    // Allow the process to take control of the terminal
    // signal(SIGTTOU, SIG_IGN); // Shell ignores SIGTTOU while child writes to terminal
    // signal(SIGTTIN, SIG_IGN); // Shell ignores SIGTTIN while child reads from terminal

    if (tcsetpgrp(STDIN_FILENO, process_group_id) == -1) {
        perror("fg: tcsetpgrp failed to give control to child");
        // This is a critical failure for foregrounding.
        // Don't proceed with kill(SIGCONT) if shell can't give terminal control.
        // Might need to reset signal handlers if they were set.
        return;
    }

    if (kill(-process_group_id, SIGCONT) == -1) { // Send SIGCONT to the entire process group
        perror("fg: kill (SIGCONT) failed");
        // Attempt to restore terminal control to shell if SIGCONT fails
        if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
            perror("fg: tcsetpgrp failed to restore control to shell (after SIGCONT fail)");
        }
        return;
    }

    fgPid = pid_to_fg; // Set global foreground PID
    printf("Bringing job [%s] (PID %d) to foreground.\n", fgCommand, pid_to_fg);


    // Remove process from background job list
    for(int j = process_index; j < bgcount - 1; j++) {
        bgs[j] = bgs[j+1]; // Struct copy
    }
    bgcount--;

    int status;
    if (waitpid(pid_to_fg, &status, WUNTRACED) == -1) {
        perror("fg: waitpid failed");
    }
    // After waitpid, fgPid is effectively no longer the shell's primary concern for this command.
    // If it was stopped, the SIGTSTP handler should have set fgPid = -1 and added to bgs.
    // If it exited/signaled, it's done.
    // So, reset fgPid here only if not already reset by a signal handler.
    // A robust way is: the signal handler sets fgPid to -1. Here we check.
    // For now, let's assume signal handler does its job for SIGTSTP. If exited/killed, it's fine.
    // The main loop's fgPid = -1 handles general reset. This is for specific fg tracking.
    // This fgPid = -1 is important if the process exited normally or was killed by a signal other than TSTP.
    if (fgPid == pid_to_fg) { // if SIGTSTP handler didn't change it
      fgPid = -1;
    }


    // Restore terminal control to the shell
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) { // getpgrp() gets current process's (shell's) PGID
        perror("fg: tcsetpgrp failed to restore control to shell");
    }
    // Restore default signal handling for TTOU/TTIN for the shell
    // signal(SIGTTOU, SIG_DFL);
    // signal(SIGTTIN, SIG_DFL);
}


// Resumes a stopped background process
void handleBg(pid_t pid_to_bg) { // Renamed pid for clarity
    if (pid_to_bg <= 0) {
        handleError("bg: Invalid PID provided.");
        return;
    }

    bool found = false;
    char process_name_for_msg[MAX_COMMAND] = "Unknown";

    for (int i = 0; i < bgcount; i++) {
        if (bgs[i].id == pid_to_bg) {
            found = true;
            // It's good to get the command name for the message before it might change
            strncpy(process_name_for_msg, bgs[i].comm, MAX_COMMAND-1);
            process_name_for_msg[MAX_COMMAND-1] = '\0';
            break;
        }
    }

    if (!found) {
        char err_msg[100];
        snprintf(err_msg, sizeof(err_msg), "bg: Job with PID %d not found in background processes.", pid_to_bg);
        handleError(err_msg);
        return;
    }

    // Send SIGCONT to the process group
    pid_t process_group_id = getpgid(pid_to_bg);
    if (process_group_id == -1) {
         // If getpgid fails, try sending to PID directly, though PG is preferred for job control.
        if (kill(pid_to_bg, SIGCONT) == -1) {
            char err_msg[150];
            snprintf(err_msg, sizeof(err_msg), "bg: kill (SIGCONT) to PID %d failed (errno %d)", pid_to_bg, errno);
            perror(err_msg); // This will print the string version of errno
            return;
        }
    } else {
        if (kill(-process_group_id, SIGCONT) == -1) { // Negative PID for process group
            char err_msg[150];
            snprintf(err_msg, sizeof(err_msg), "bg: kill (SIGCONT) to PGID %d failed (errno %d)", process_group_id, errno);
            perror(err_msg);
            return;
        }
    }

    printf("Resuming job [%s] (PID %d) in background.\n", process_name_for_msg, pid_to_bg);
    // The process remains in the bgs list.
}