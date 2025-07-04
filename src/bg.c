// _POSIX_C_SOURCE is defined in Makefile CFLAGS
// System headers (stdio, stdlib, string, stdbool, signal, unistd, sys/wait)
// are expected to be included via ../utils/global.h, which is pulled in by ../include/bg.h.

#include "../include/bg.h"          // Project-specific header for bg functionality
#include "../utils/error_handler.h" // For handleError
#include "../utils/string_utils.h"  // For trimstr
#include "../include/IO_redirection.h" // For handleIO, resetIO
#include "../include/syscmd.h"      // For systemComamnd


// Forward declarations for static functions
static char *get_bg_process_name(pid_t pid);
static void delete_bg_process_entry(pid_t pid);


void bg(char *command_input_string) // Renamed subcom_input for clarity
{
    if (command_input_string == NULL) {
        handleError("NULL command passed to bg");
        return;
    }

    char *command_to_execute = strdup(command_input_string);
    if (command_to_execute == NULL) {
        handleError("strdup failed in bg for command_to_execute");
        return;
    }

    // Handle I/O redirection for the background command
    // handleIO returns a new string that is the command part, or NULL on error.
    char *command_after_io = handleIO(command_to_execute);
    // N.B.: handleIO currently takes char* and returns char*. It should ideally take const char* if it doesn't modify its input,
    // or the caller should be aware. The current handleIO seems to expect to modify its input via strtok.
    // The strdup above for command_to_execute makes a mutable copy.

    if (command_after_io == NULL) { // Error in handleIO or no command part left
        // handleError is called within handleIO if there's an issue.
        // If command_after_io is NULL, it means handleIO failed to extract a command.
        free(command_to_execute); // Free the initial strdup
        // resetIO(); // Important: resetIO must be called if handleIO was partially successful
        return;
    }
    // The original command_to_execute is not needed anymore as command_after_io is the processed one.
    // However, command_after_io points to memory allocated by handleIO, which must be freed.
    // Let's make command_to_execute point to what command_after_io returned, and free the old command_to_execute.
    free(command_to_execute);
    command_to_execute = command_after_io; // Now command_to_execute holds the string from handleIO

    trimstr(command_to_execute); // Trim whitespace from the final command
    if (strlen(command_to_execute) == 0) {
        handleError("Empty command after I/O redirection for bg");
        free(command_to_execute);
        resetIO(); // Ensure IO is reset
        return;
    }

    pid_t child_pid = fork();

    if (child_pid == -1)
    {
        handleError("fork failed for background process");
        free(command_to_execute);
        resetIO(); // Reset I/O as fork failed before exec
        return;
    }
    else if (child_pid == 0)
    { // Child process
        setpgrp(); // Detach from terminal's process group
        // systemComamnd expects a mutable string for strtok
        // command_to_execute is already a mutable string from handleIO (strdup)
        systemComamnd(command_to_execute); // Execute the command
        // If systemComamnd returns (i.e., execvp fails), exit child
        // Error is handled in systemComamnd
        exit(EXIT_FAILURE);
    }
    else
    { // Parent process
        if (bgcount < MAX_BG_PROCESSES) {
            bgs[bgcount].id = child_pid;
            strncpy(bgs[bgcount].comm, command_to_execute, MAX_COMMAND -1);
            bgs[bgcount].comm[MAX_COMMAND-1] = '\0';
            bgcount++;
            dprintf(stdout_copy_fd, "[%d] %d %s\n", bgcount, child_pid, command_to_execute); // Notify user
        } else {
            handleError("Maximum background processes limit reached");
            // Optionally kill the new child process if it can't be tracked
            kill(child_pid, SIGKILL);
        }
    }

    resetIO(); // Reset I/O redirection for the shell
    free(command_to_execute); // Free the command string processed by handleIO
}

// Renamed from delete_bg for clarity
static void delete_bg_process_entry(pid_t pid)
{
    int i;
    bool found = false;
    for (i = 0; i < bgcount; i++)
    {
        if (bgs[i].id == pid)
        {
            found = true;
            break;
        }
    }

    if (found) {
        // Shift remaining elements left
        for (int j = i; j < bgcount - 1; j++)
        {
            bgs[j] = bgs[j + 1]; // struct copy
        }
        bgcount--;
    }
}

// Renamed from getname for clarity and made static
static char *get_bg_process_name(pid_t pid)
{
    for (int i = 0; i < bgcount; i++)
    {
        if (bgs[i].id == pid)
        {
            return bgs[i].comm;
        }
    }
    return "UnknownProcess"; // Return a placeholder if not found
}

void checkbg()
{
    int status;
    pid_t terminated_pid;
    // WNOHANG ensures this call is non-blocking
    while ((terminated_pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        char *name = get_bg_process_name(terminated_pid);
        if (WIFEXITED(status))
        {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0)
            {
                dprintf(stdout_copy_fd, COLOR_MAGENTA "%s (PID %d)" COLOR_RESET " exited normally.\n", name, terminated_pid);
            }
            else
            {
                dprintf(stdout_copy_fd, COLOR_MAGENTA "%s (PID %d)" COLOR_RESET " exited abnormally with status %d.\n", name, terminated_pid, exit_status);
            }
        }
        else if (WIFSIGNALED(status))
        {
            dprintf(stdout_copy_fd, COLOR_MAGENTA "%s (PID %d)" COLOR_RESET " terminated by signal %d.\n", name, terminated_pid, WTERMSIG(status));
        }
        // Regardless of how it terminated, remove it from the list
        delete_bg_process_entry(terminated_pid);
    }
}
