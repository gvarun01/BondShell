#include "../include/activities.h"
#include "../utils/error_handler.h" // For handleError
#include <stdio.h>  // For snprintf, fopen, fclose, fscanf, printf
#include <stdlib.h> // For qsort
#include <string.h> // For strcpy

// Comparison function for qsort, sorts by process ID
static int compareProcessID(const void *a, const void *b) {
    return ((struct bgprocess *)a)->id - ((struct bgprocess *)b)->id;
}

void activities() {
    if (bgcount == 0) {
        printf("No background activities.\n");
        return;
    }

    // Sort background processes by PID for consistent output
    // Note: qsort modifies the bgs array in place. If original order is important elsewhere, make a copy.
    qsort(bgs, bgcount, sizeof(struct bgprocess), compareProcessID);

    printf("Running/Stopped Background Activities:\n");
    for (int i = 0; i < bgcount; i++) {
        int pid_from_bgs_entry = bgs[i].id; // Use a different variable name to avoid confusion with pid from fscanf
        char current_process_status_str[32]; // Renamed from current_process_state for clarity
        char proc_stat_path[256]; // Renamed from procPath
        snprintf(proc_stat_path, sizeof(proc_stat_path), "/proc/%d/stat", pid_from_bgs_entry);

        FILE *stat_file_ptr = fopen(proc_stat_path, "r"); // Renamed fp
        if (stat_file_ptr == NULL) {
            char err_msg[MAX_PATH + 50];
            snprintf(err_msg, sizeof(err_msg), "activities: Failed to open /proc/%d/stat", pid_from_bgs_entry);
            // perror(err_msg); // Using perror provides more specific system error
            handleError(err_msg); // Or stick to handleError for consistency
            continue;
        }

        // Variables to hold data read from /proc/[pid]/stat
        int pid_read; // PID read from stat file (should match pid_from_bgs_entry)
        char process_name[256];
        char process_char_state; // Single char representing state
        int ppid, pgid_stat, session_id, tty_num, tpgid_stat; // Renamed pgrp to pgid_stat, session to session_id, etc.

        // Expected number of items to be read by fscanf
        int expected_fscanf_items = 8;
        int items_read = fscanf(stat_file_ptr, "%d %s %c %d %d %d %d %d",
                                &pid_read, process_name, &process_char_state,
                                &ppid, &pgid_stat, &session_id, &tty_num, &tpgid_stat);

        fclose(stat_file_ptr); // Close file as soon as data is read or attempt to read is made

        if (items_read != expected_fscanf_items) {
            char err_msg[MAX_PATH + 100];
            snprintf(err_msg, sizeof(err_msg), "activities: Failed to parse /proc/%d/stat correctly (read %d of %d items)",
                     pid_from_bgs_entry, items_read, expected_fscanf_items);
            handleError(err_msg);
            continue; // Skip this entry if parsing failed
        }

        // Determine display string for process state
        if (process_char_state == 'T' || process_char_state == 't') { // 't' for tracing stop
            strcpy(current_process_status_str, "Stopped");
        } else if (process_char_state == 'Z') {
             strcpy(current_process_status_str, "Zombie");
        } else if (process_char_state == 'D') {
             strcpy(current_process_status_str, "Disk Sleep");
        }
        // Add more states if needed: R (Running), S (Sleeping), I (Idle)
        else { // Consider S, R, I as "Running" for simplicity in this context
            strcpy(current_process_status_str, "Running");
        }
        // Using bgs[i].comm which was stored when the process was backgrounded.
        // process_name from /proc/[pid]/stat might be truncated or different.
        printf("%d : %s - %s\n", pid_from_bgs_entry, bgs[i].comm, current_process_status_str);
    }
}