#include "../include/signal.h"
#include "../utils/error_handler.h" // For handleError
#include <stdio.h>  // For printf, perror
#include <string.h> // For strtok, strcpy
#include <stdlib.h> // For atoi
#include <signal.h> // For kill, SIGINT, SIGKILL, SIGTSTP
#include <errno.h>  // For errno, ESRCH
#include <unistd.h> // For setpgid (though might need sys/types.h too for pid_t)


// Sends a specified signal to a process.
void ping(char *command_args_str) // Renamed subcom for clarity
{
    if (command_args_str == NULL) {
        handleError("ping: Missing arguments. Usage: ping <pid> <signal_number>");
        return;
    }
    char *args_copy = strdup(command_args_str);
    if(args_copy == NULL) {
        handleError("ping: strdup failed.");
        return;
    }

    char *tokenizer_context;
    char *command_name = strtok_r(args_copy, " \t\n", &tokenizer_context); // "ping"
    char *pid_str = strtok_r(NULL, " \t\n", &tokenizer_context);
    char *signal_num_str = strtok_r(NULL, " \t\n", &tokenizer_context);

    if (pid_str == NULL || signal_num_str == NULL)
    {
        handleError("ping: Missing PID or signal number. Usage: ping <pid> <signal_number>");
        free(args_copy);
        return;
    }

    pid_t target_pid = atoi(pid_str);
    int signal_to_send = atoi(signal_num_str);

    if (target_pid <= 0) {
        handleError("ping: Invalid PID.");
        free(args_copy);
        return;
    }
    // Signal 0 can be used to check if a process exists.
    // Other signals are typically positive.
    if (signal_to_send < 0 || signal_to_send >= NSIG) { // NSIG is max signal number
        handleError("ping: Invalid signal number.");
        free(args_copy);
        return;
    }


    if (kill(target_pid, signal_to_send) == -1)
    {
        if (errno == ESRCH) // No such process
        {
            char err_msg[100];
            snprintf(err_msg, sizeof(err_msg), "ping: No such process with PID %d.", target_pid);
            handleError(err_msg);
        }
        else // Other error, e.g., permission denied
        {
            char err_msg[100];
            snprintf(err_msg, sizeof(err_msg), "ping: Failed to send signal %d to PID %d", signal_to_send, target_pid);
            perror(err_msg); // This will append the system error message (e.g., "Permission denied")
        }
    }
    else
    {
        printf("Sent signal %d to process with PID %d.\n", signal_to_send, target_pid);
    }
    free(args_copy);
}

// Signal handler for SIGINT (Ctrl+C).
void handleCtrlC(int sig_num) // Renamed sig to sig_num
{
    if (fgPid != -1) // If there's a foreground process controlled by the shell
    {
        // Send SIGINT to the entire foreground process group
        if (kill(-fgPid, SIGINT) == -1) // Assuming fgPid is also the PGID for simple foreground jobs
        {
            // If kill by PGID fails, try PID directly (though less ideal for job control)
            if (kill(fgPid, SIGINT) == -1 && errno != ESRCH) { // Avoid error if process already gone
                 perror("handleCtrlC: kill failed");
            }
        }
        // Don't print "Sent SIGINT..." here, as the receiving process might print its own message or terminate silently.
        // The visual feedback is the command terminating or the prompt reappearing.
        // fgPid = -1; // fgPid should be reset by the main loop or waitpid logic when command terminates.
                      // Or, if the command handles SIGINT and continues, fgPid remains.
                      // For typical Ctrl+C, command terminates, so waitpid in executeInFG would clear it.
    } else {
        // If no foreground process, Ctrl+C should typically just show a new prompt line.
        // Or, if the shell itself should terminate on Ctrl+C when input is empty, add that logic here.
        // For now, just a newline to make it look like a new prompt is ready.
        printf("\n");
        // Potentially call display_prompt here if the main loop isn't guaranteed to do it next.
    }
    // Re-register signal handler for next time (some systems might require this)
    signal(SIGINT, handleCtrlC);
}

// Kills all tracked background processes and the current foreground process.
// Typically called when the shell exits.
void killAllProcesses()
{
    if (fgPid != -1)
    {
        if (kill(-fgPid, SIGKILL) == -1) // Kill entire foreground process group
        {
            if (errno != ESRCH) // Ignore "No such process"
            {
                // perror("killAllProcesses: Failed to kill foreground process group");
                // Try killing just the PID if group kill fails
                if (kill(fgPid, SIGKILL) == -1 && errno != ESRCH) {
                    // perror("killAllProcesses: Failed to kill foreground process PID");
                }
            }
        }
        fgPid = -1; // Mark as killed
    }

    for (int i = 0; i < bgcount; i++)
    {
        if (bgs[i].id > 0) { // Ensure valid PID
             if (kill(-bgs[i].id, SIGKILL) == -1) { // Kill entire background job's process group
                if (errno != ESRCH) {
                    // perror("killAllProcesses: Failed to kill background process group");
                    if (kill(bgs[i].id, SIGKILL) == -1 && errno != ESRCH) {
                        // perror("killAllProcesses: Failed to kill background process PID");
                    }
                }
            }
        }
    }
    bgcount = 0; // Clear background process list
}

// Signal handler for SIGTSTP (Ctrl+Z).
void handleCtrlZ(int sig_num) { // Renamed sig to sig_num
    if (fgPid != -1) { // If there's a foreground process
        // Send SIGTSTP to the foreground process group
        if (kill(-fgPid, SIGTSTP) == -1) { // Assuming fgPid is also PGID
            // If group kill fails, try PID directly
            if (kill(fgPid, SIGTSTP) == -1 && errno != ESRCH) {
                 perror("handleCtrlZ: kill (SIGTSTP) failed");
                 return; // Don't add to background jobs if we couldn't stop it.
            } else if (errno == ESRCH) { // Process already gone
                fgPid = -1;
                return;
            }
        }

        // Add the stopped process to the background jobs list
        if (bgcount < MAX_BG_PROCESSES) {
            // Ensure the process has its own process group ID, if not already set.
            // This should ideally happen when the process is forked if it's an external command.
            // For simplicity, if using `setpgid(0,0)` in child exec path, fgPid is already a PGID.
            // If not, `setpgid(fgPid, fgPid)` here would try to make it a PG leader.
            // This can fail if fgPid is not the calling process or child of calling process.
            // Let's assume job control is simple: fgPid is the process group leader.

            bgs[bgcount].id = fgPid;
            // fgCommand should already hold the command string
            if(fgCommand != NULL) {
                strncpy(bgs[bgcount].comm, fgCommand, MAX_COMMAND - 1);
                bgs[bgcount].comm[MAX_COMMAND - 1] = '\0';
            } else {
                strcpy(bgs[bgcount].comm, "Unknown FG Job");
            }
            bgcount++;
            printf("\n[%d]+  Stopped                 %s\n", bgcount, bgs[bgcount-1].comm);
        } else {
            handleError("handleCtrlZ: Maximum background processes limit reached. Cannot move stopped process to background.");
            // Process was stopped but not added to bg list. It's now a zombie or needs manual handling.
            // Could send SIGKILL to it if not trackable.
        }

        fgPid = -1; // Process is no longer in foreground from shell's perspective
    }
    // Re-register handler
    signal(SIGTSTP, handleCtrlZ);
}
