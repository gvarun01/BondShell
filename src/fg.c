#include "../include/fg.h"
#include "../utils/error_handler.h" // For handleError
#include "../utils/shell_utils.h"   // For exitCheck
#include "../utils/string_utils.h"  // For trimstr
#include "../utils/path_utils.h"    // For pathCorrect
// Standard library headers that might be needed directly
#include <stdio.h>  // For sscanf, printf (though printf is often for debugging)
#include <string.h> // For strcmp, strncmp, strcpy, strlen, strtok_r
#include <stdlib.h> // For malloc, free, exit, getenv
#include <unistd.h> // For chdir, getcwd, fork, execvp, setpgid, tcsetpgrp, waitpid

// executeInFG is the main dispatcher for commands that run in the foreground.
void executeInFG(char *command_input_str) // Renamed command_input for clarity
{
    if (command_input_str == NULL) {
        handleError("NULL command passed to executeInFG");
        resetIO(); // Ensure I/O is reset even if command is NULL
        return;
    }

    // Make a mutable copy for processing (strtok, etc.)
    char *command_buffer = strdup(command_input_str);
    if (command_buffer == NULL) {
        handleError("strdup failed in executeInFG for command_buffer");
        resetIO();
        return;
    }

    // Handle I/O Redirection. handleIO returns a new string with only the command part.
    char *command_after_io = handleIO(command_buffer);
    // command_buffer was passed to handleIO, which might have tokenized it or used it.
    // The string returned by handleIO is what we should use.

    if (command_after_io == NULL) {
        // Error handled in handleIO. We must free command_buffer then return.
        free(command_buffer);
        resetIO(); // Crucial: reset I/O if redirection setup failed or command was invalid.
        return;
    }
    // Now, command_after_io holds the actual command to execute. Free the original command_buffer.
    free(command_buffer);
    command_buffer = command_after_io; // command_buffer now points to the string from handleIO

    trimstr(command_buffer); // Trim whitespace

    if (strlen(command_buffer) == 0) { // If command is empty after IO handling and trimming
        free(command_buffer);
        resetIO();
        return;
    }

    // Set the global fgCommand, used by signal handlers (e.g., Ctrl+Z)
    // Ensure fgCommand is large enough or dynamically sized.
    // MAX_COMMAND should be sufficient as per global.h.
    strncpy(fgCommand, command_buffer, MAX_COMMAND -1);
    fgCommand[MAX_COMMAND-1] = '\0';


    // Crucial: exitCheck must be called before attempting to execute other commands
    // to ensure "exit" command works correctly.
    exitCheck(command_buffer); // exitCheck handles its own strdup and trimming

    // --- Built-in command dispatch ---
    // Order matters: more specific commands (like "log purge") before general ones ("log").

    if (strcmp(command_buffer, "log") == 0)
    {
        printLog();
    }
    else if (strcmp(command_buffer, "log purge") == 0)
    {
        purgeLog();
    }
    else if (strncmp(command_buffer, "log execute ", 12) == 0) // Note space after execute
    {
        int index_to_execute;
        // Use %d with space before to skip whitespace: "log execute   5"
        if (sscanf(command_buffer + strlen("log execute"), " %d", &index_to_execute) == 1) {
            char *cmd_from_log = executeLog(index_to_execute);
            if (cmd_from_log == NULL)
            {
                handleError("log execution failed: command not found or invalid index.");
            }
            else
            {
                // cmd_from_log is from strdup in executeLog, needs freeing by caller if not used by parser
                // parser itself should handle memory of its input string.
                // The old code had `cmd[strlen(cmd) - 1] = '\0';` which implies log stores with newline.
                // Assuming executeLog returns a clean command string (without newline).
                // If parser copies the string, cmd_from_log should be freed here.
                // For now, assume parser uses it or copies it.
                // Let's ensure it's null-terminated if it had a newline.
                size_t len = strlen(cmd_from_log);
                if (len > 0 && cmd_from_log[len-1] == '\n') cmd_from_log[len-1] = '\0';

                // Recursive call to parser could be problematic if not handled carefully (e.g. depth).
                // Consider a command queue or simpler execution model for log items.
                // For now, keeping original behavior:
                parser(cmd_from_log);
                free(cmd_from_log); // Free after parser is done with it.
            }
        } else {
            handleError("log execute: invalid or missing index.");
        }
    }
    else if (strncmp(command_buffer, "cd", 2) == 0 && (command_buffer[2] == ' ' || command_buffer[2] == '\0'))
    {
        char *path_argument = NULL;
        if (command_buffer[2] == ' ') { // Arguments exist
            path_argument = command_buffer + 3;
            trimstr(path_argument); // Trim the argument part
        }

        char current_dir_before_cd[MAX_PATH];
        if (getcwd(current_dir_before_cd, sizeof(current_dir_before_cd)) == NULL)
        {
            handleError("getcwd failed before cd operation");
            // Not returning, will attempt chdir anyway but lastDir might be incorrect.
            current_dir_before_cd[0] = '\0'; // Mark as invalid
        }

        char target_path_buffer[MAX_PATH * 2]; // Buffer for constructing path
        bool change_successful = false;

        if (path_argument == NULL || strlen(path_argument) == 0) // "cd" or "cd "
        {
            if (chdir(home) == 0) change_successful = true;
            else handleError("chdir to home directory failed");
        }
        else if (strcmp(path_argument, "-") == 0)
        {
            if (strlen(lastDir) == 0)
            {
                handleError("cd: OLDPWD not set");
            }
            else
            {
                // Print the directory being changed to, as per POSIX `cd -`
                printf("%s\n", lastDir);
                if (chdir(lastDir) == 0) change_successful = true;
                else handleError("chdir to last directory failed");
            }
        }
        else
        {
            char *corrected_path = pathCorrect(path_argument);
            if (corrected_path != NULL) {
                if (chdir(corrected_path) == 0) change_successful = true;
                else {
                    char err_msg[MAX_PATH + 50];
                    snprintf(err_msg, sizeof(err_msg), "cd: Could not change to directory: %s", corrected_path);
                    handleError(err_msg);
                }
                free(corrected_path);
            } else {
                handleError("cd: Invalid path argument after correction");
            }
        }

        if (change_successful && strlen(current_dir_before_cd) > 0) {
            strcpy(lastDir, current_dir_before_cd); // Update OLDPWD (lastDir)
        }
    }
    else if (strncmp(command_buffer, "help", 4) == 0 && (command_buffer[4] == '\0' || command_buffer[4] == ' ')) // "help" or "help bond" etc.
    {
        displayHelp();
    }
    else if (strncmp(command_buffer, "iMan ", 5) == 0) // Requires an argument
    {
        iman(command_buffer); // iman function itself parses its argument
    }
    else if (strncmp(command_buffer, "ping ", 5) == 0) // Requires arguments
    {
        ping(command_buffer); // ping function parses its arguments
    }
    else if (strcmp(command_buffer, "activities") == 0)
    {
        activities();
    }
    else if (strncmp(command_buffer, "fg ", 3) == 0) // Requires PID
    {
        char *pid_str_arg = command_buffer + 3;
        trimstr(pid_str_arg);
        pid_t pid_to_fg = atoi(pid_str_arg); // atoi returns 0 on error, check if pid_str_arg is valid num
        if (pid_to_fg > 0) { // Basic validation
            handleFg(pid_to_fg);
        } else {
            handleError("fg: Invalid PID provided.");
        }
    }
    else if (strncmp(command_buffer, "bg ", 3) == 0) // Requires PID
    {
        char *pid_str_arg = command_buffer + 3;
        trimstr(pid_str_arg);
        pid_t pid_to_bg = atoi(pid_str_arg);
        if (pid_to_bg > 0) {
            handleBg(pid_to_bg);
        } else {
            handleError("bg: Invalid PID provided.");
        }
    }
    else if (strncmp(command_buffer, "neonate ", 8) == 0) // Requires arguments
    {
        neonate(command_buffer + 8); // neonate parses its own args
    }
    else if (strncmp(command_buffer, "proclore", 8) == 0 && (command_buffer[8] == ' ' || command_buffer[8] == '\0'))
    {
        pid_t pid_to_proclore = 0; // Default to current shell if no arg
        if (command_buffer[8] == ' ') {
            char *pid_str_arg = command_buffer + 9;
            trimstr(pid_str_arg);
            if(strlen(pid_str_arg) > 0) pid_to_proclore = atoi(pid_str_arg);
            if (pid_to_proclore <=0 && strlen(pid_str_arg) > 0) { // atoi failed or gave non-positive
                 handleError("proclore: Invalid PID provided.");
                 pid_to_proclore = -1; // Indicate error
            }
        }
        if(pid_to_proclore != -1) proclore(pid_to_proclore);
    }
    else if (strncmp(command_buffer, "reveal", 6) == 0 && (command_buffer[6] == ' ' || command_buffer[6] == '\0'))
    {
        reveal(command_buffer + 7); // reveal parses its own args
    }
    else if (strncmp(command_buffer, "seek", 4) == 0 && (command_buffer[4] == ' ' || command_buffer[4] == '\0'))
    {
        seek(command_buffer + 5); // seek parses its own args
    }
    else if (strncmp(command_buffer, "hop", 3) == 0 && (command_buffer[3] == ' ' || command_buffer[3] == '\0'))
    {
        if(strcmp(command_buffer, "hop") == 0){ // "hop" with no args
            hop(NULL);
        }
        else{ // "hop <args>"
            hop(command_buffer + 4);
        }
    }
    else // External command
    {
        // This part needs command_buffer to be tokenized for execvp
        char *args[MAX_COMMAND / 2 + 1]; // Max possible arguments
        char *tokenizer_context_exec;
        char *token_for_exec = strtok_r(command_buffer, " \t\n", &tokenizer_context_exec);
        int arg_idx = 0;
        while (token_for_exec != NULL && arg_idx < MAX_COMMAND / 2)
        {
            args[arg_idx++] = token_for_exec;
            token_for_exec = strtok_r(NULL, " \t\n", &tokenizer_context_exec);
        }
        args[arg_idx] = NULL; // Null-terminate the argument list

        if (arg_idx == 0) { // Empty command after all parsing.
            // This case should ideally be caught earlier.
        } else {
            pid_t child_pid = fork();
            if (child_pid == -1) {
                handleError("fork failed for external command");
            } else if (child_pid == 0) // Child process
            {
                // For external commands, ensure they run in their own process group if interactive
                // (like nano, vim) to handle signals correctly.
                // Non-interactive commands typically don't need this.
                // The original code had a check for nano/vi/vim.
                // A more general approach is to always setpgid for child processes
                // that are intended to be interactive or long-running foreground tasks.
                // However, for simple commands, this might be overkill or even slightly incorrect
                // if they are part of a pipeline that the shell manages.
                // For now, let's replicate the specific check for editors.
                if(strncmp(args[0], "nano" , 4) != 0 &&
                   strncmp(args[0], "vi" , 2) != 0 &&
                   strncmp(args[0], "vim" , 3) != 0){
                    setpgid(0, 0); // Make this child its own process group leader
                }

                if (execvp(args[0], args) < 0)
                {
                    char err_msg[MAX_COMMAND + 50];
                    snprintf(err_msg, sizeof(err_msg), "Execution failed for command '%s'", args[0]);
                    handleError(err_msg); // More specific error
                    // perror(args[0]); // Also useful for system error
                    exit(EXIT_FAILURE); // Exit child if execvp fails
                }
                // fflush(stdout) here is usually not needed as execvp replaces the process image.
            }
            else // Parent process
            {
                fgPid = child_pid; // Set global foreground PID for signal handling

                int status;
                waitpid(child_pid, &status, WUNTRACED); // Wait for child to change state

                // After child finishes or is stopped, restore terminal control to the shell
                // signal(SIGTTOU, SIG_IGN); // Ignore TTOU when shell tries to grab terminal
                // tcsetpgrp(STDIN_FILENO, getpgrp()); // Give terminal control back to shell's process group
                // signal(SIGTTOU, SIG_DFL); // Restore default TTOU handling

                if(WIFSTOPPED(status)){ // If child was stopped (e.g. Ctrl+Z)
                    // This means handleCtrlZ in main should have handled adding it to bgs.
                    // fgPid should be reset by handleCtrlZ.
                    // Here, we just acknowledge it.
                     dprintf(stdout_copy_fd, "\nProcess %d (%s) stopped.\n", fgPid, fgCommand);
                     // The signal handler for SIGTSTP should add it to background jobs.
                } else if (WIFEXITED(status)) {
                    if (WEXITSTATUS(status) != 0) {
                        // Optional: print exit status for failed commands
                        // printf("Process %d exited with status %d\n", child_pid, WEXITSTATUS(status));
                    }
                } else if (WIFSIGNALED(status)) {
                     // Optional: print signal that terminated the command
                     // printf("Process %d terminated by signal %d\n", child_pid, WTERMSIG(status));
                }
                fgPid = -1; // Reset fgPid as the foreground process is no longer active under shell's direct wait
            }
        }
    }

    free(command_buffer); // Free the string that was processed (originally from handleIO or strdup)
    resetIO(); // Reset I/O settings (stdin, stdout) for the next command prompt
}
