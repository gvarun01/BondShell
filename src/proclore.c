#include "../include/proclore.h"
#include "../utils/error_handler.h" // For handleError
#include "../utils/path_utils.h"   // For relativePath
#include <stdio.h>      // For sprintf, FILE, fopen, fgets, fclose, sscanf, printf
#include <string.h>     // For strncmp
#include <stdlib.h>     // For getpid (though unistd.h is more common)
#include <unistd.h>     // For getpid, getpgid, tcgetpgrp, readlink, STDIN_FILENO
#include <sys/types.h>  // For pid_t

void proclore(pid_t target_pid) // Renamed pid to target_pid for clarity
{
    if (target_pid == 0) // If PID is 0, use current shell's PID
    {
        target_pid = getpid();
    }
    if (target_pid < 0) { // Basic validation
        handleError("proclore: Invalid PID provided.");
        return;
    }


    char process_state = '?';         // Default to unknown
    unsigned long vm_size_kb = 0;     // Virtual memory size in KB
    pid_t process_group_id = -1;      // Process group ID
    char foreground_background_char = ' '; // '+' if foreground, ' ' otherwise

    char status_file_path[MAX_PATH];
    snprintf(status_file_path, sizeof(status_file_path), "/proc/%d/status", target_pid);

    FILE *status_file = fopen(status_file_path, "r");
    if (status_file == NULL)
    {
        char err_msg[MAX_PATH + 50];
        snprintf(err_msg, sizeof(err_msg), "proclore: Failed to open status file for PID %d", target_pid);
        // perror(err_msg); // Using perror can give more system-specific error info
        handleError(err_msg); // Or stick to handleError for consistency
        return;
    }

    char line_buffer[256]; // Lines in /proc/[pid]/status are generally not too long
    while (fgets(line_buffer, sizeof(line_buffer), status_file))
    {
        if (strncmp(line_buffer, "State:", 6) == 0)
        {
            // Example: "State:  S (sleeping)" - sscanf needs to skip whitespace and get the char
            sscanf(line_buffer + 6, " %c", &process_state); // Note space in " %c"
        }
        else if (strncmp(line_buffer, "VmSize:", 7) == 0)
        {
            sscanf(line_buffer + 7, "%lu", &vm_size_kb); // VmSize is in kB
        }
        else if (strncmp(line_buffer, "Tgid:", 5) == 0) // Thread Group ID (effectively PID for main thread)
        {
            // This is actually the PID itself, not the process group ID for job control.
            // PGID is obtained using getpgid().
            // For this display, let's use PGID.
        }
         else if (strncmp(line_buffer, "PPid:", 5) == 0) { /* ... */ }
         // Add more fields if needed, e.g., PPid, Uid, Gid etc.
    }
    fclose(status_file);

    // Get Process Group ID
    process_group_id = getpgid(target_pid);
    if (process_group_id == -1) {
        // perror("proclore: getpgid failed");
        // Not critical enough to stop, just won't display PGID or foreground status correctly.
    }

    // Check if it's a foreground process in the current terminal
    pid_t terminal_foreground_pgid = tcgetpgrp(STDIN_FILENO);
    if (terminal_foreground_pgid != -1 && process_group_id != -1 && process_group_id == terminal_foreground_pgid) {
        foreground_background_char = '+';
    }

    // Get executable path
    char executable_path_buffer[MAX_PATH] = "N/A"; // Default if readlink fails
    char proc_exe_link_path[MAX_PATH];
    snprintf(proc_exe_link_path, sizeof(proc_exe_link_path), "/proc/%d/exe", target_pid);

    ssize_t path_len = readlink(proc_exe_link_path, executable_path_buffer, sizeof(executable_path_buffer) - 1);
    if (path_len != -1)
    {
        executable_path_buffer[path_len] = '\0'; // Null-terminate the path
    } else {
        // Error reading link, executable_path_buffer remains "N/A" or contains partial data.
        // perror("proclore: readlink failed for executable path");
        // For some processes (e.g. kernel threads), /proc/[pid]/exe might not be accessible.
        strncpy(executable_path_buffer, "N/A (permission or non-existent)", sizeof(executable_path_buffer)-1);
        executable_path_buffer[sizeof(executable_path_buffer)-1] = '\0';
    }

    char *display_path = relativePath(executable_path_buffer); // Convert to path relative to home if applicable
    if (display_path == NULL) { // If relativePath fails (e.g. malloc)
        display_path = strdup(executable_path_buffer); // Fallback to the original buffer's content
        if (display_path == NULL) display_path = "Error getting path"; // Ultimate fallback
    }


    printf("PID: %d\n", target_pid);
    printf("Status: %c%c\n", process_state, foreground_background_char);
    if (process_group_id != -1) {
        printf("Process Group ID: %d\n", process_group_id);
    } else {
        printf("Process Group ID: N/A\n");
    }
    printf("Virtual Memory: %lu kB\n", vm_size_kb);
    printf("Executable Path: %s\n", display_path);

    if (display_path != executable_path_buffer && strcmp(display_path, "Error getting path") != 0) {
         free(display_path); // Free if relativePath or strdup allocated new memory
    } else if (display_path == executable_path_buffer && strcmp(display_path, "Error getting path") == 0) {
        // This means strdup failed after relativePath failed, so display_path points to the literal string.
        // No free needed for the literal string.
    } else if (strcmp(display_path, "Error getting path") == 0){
        // also no free
    }

}