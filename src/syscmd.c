#include "../include/syscmd.h"
#include "../utils/error_handler.h" // For handleError
#include <stdio.h>  // For printf (used for status messages, consider removing for cleaner API)
#include <stdlib.h> // For malloc, free (though current version might not need if not duplicating args)
#include <string.h> // For strtok, strcmp, strncmp, strcpy, strcat
#include <unistd.h> // For getcwd, execvp
#include <sys/wait.h> // For WIFEXITED, WEXITSTATUS, WIFSIGNALED (though execvp replaces process, so parent waits)

// Executes a system command. This function is typically called from within a child process after fork.
// If execvp is successful, this function does not return.
// If execvp fails, it returns 1 (or -1, standardizing return codes is good).
// The original function had some printf statements for success/failure, which are unusual
// for a function that's meant to be part of an exec call in a child.
// These are removed as the parent process usually handles status reporting after waitpid.
int systemComamnd(char *command_string) // Renamed command for clarity
{
    if (command_string == NULL || strlen(command_string) == 0) {
        handleError("systemComamnd: No command provided.");
        return -1; // Indicate failure
    }

    // strtok modifies the string, so command_string must be mutable.
    // The caller (e.g., bg.c or executeInFG) must ensure this.
    // Typically, a copy is made before calling if original string is needed later.

    char *args[MAX_COMMAND / 2 + 1]; // Max possible arguments, assuming avg 2 chars per arg + space
    int arg_count = 0;
    char *tokenizer_context; // For strtok_r if preferred, but simple strtok is used here
    char *token = strtok(command_string, " \t\n");

    // Buffers for paths that need dynamic allocation (e.g. involving home dir or getcwd)
    // These need to be freed after execvp if it fails, or managed carefully.
    // For simplicity, let's assume that if execvp succeeds, memory is reclaimed by OS.
    // If it fails, we need to free. This is tricky.
    // A safer way is to have args point to parts of a single duplicated command_string,
    // and only allocate for special cases like "~" expansion.
    char *allocated_paths[MAX_COMMAND / 2 + 1] = {NULL}; // Track allocated strings for cleanup
    int allocated_count = 0;


    while (token != NULL && arg_count < MAX_COMMAND / 2)
    {
        if (strcmp(token, "~") == 0)
        {
            // Home directory path is global, no new allocation needed if args[i] just points to it.
            // However, execvp expects array of char*, where some might be literals and some allocated.
            // If 'home' is a global char array, this is fine.
            args[arg_count] = home;
        }
        else if (strncmp(token, "~/", 2) == 0) // Handle paths like ~/Documents
        {
            char *expanded_path = (char *)malloc(MAX_PATH);
            if (expanded_path == NULL) {
                handleError("systemComamnd: malloc failed for ~ path expansion");
                // Cleanup previously allocated_paths
                for(int k=0; k < allocated_count; ++k) free(allocated_paths[k]);
                return -1;
            }
            strcpy(expanded_path, home);
            strcat(expanded_path, token + 1); // Append from '/' onwards
            args[arg_count] = expanded_path;
            allocated_paths[allocated_count++] = expanded_path;
        }
        // The original code had a case for ".", which would resolve to CWD.
        // Most shells let execvp handle "." directly by searching PATH or CWD.
        // Explicitly resolving "." to an absolute path might be desired in some contexts
        // but can also be omitted for simplicity if PATH handling is standard.
        // For now, removing the explicit "." handling to rely on execvp's behavior.
        /*
        else if (strcmp(token, ".") == 0)
        {
            char *cwd_path = (char *)malloc(MAX_PATH);
            if (getcwd(cwd_path, MAX_PATH) == NULL)
            {
                handleError("systemComamnd: getcwd failed for '.' expansion");
                free(cwd_path); // Should be done only if getcwd fails AFTER malloc
                for(int k=0; k < allocated_count; ++k) free(allocated_paths[k]);
                return -1;
            }
            args[arg_count] = cwd_path;
            allocated_paths[allocated_count++] = cwd_path;
        }
        */
        else
        {
            args[arg_count] = token; // Points directly into the (modified) command_string
        }
        arg_count++;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL; // Null-terminate the argument list for execvp

    if (arg_count == 0) { // No command found after tokenization
        handleError("systemComamnd: Empty command after tokenization.");
         for(int k=0; k < allocated_count; ++k) free(allocated_paths[k]);
        return -1;
    }

    // fflush(stdout) before execvp is a good practice if there was any prior output from shell
    // that needs to appear before the new program's output.
    fflush(stdout);
    fflush(stderr);

    execvp(args[0], args); // Execute the command

    // If execvp returns, an error occurred.
    char error_message[MAX_PATH + 50];
    snprintf(error_message, sizeof(error_message), "systemComamnd: execvp failed for '%s'", args[0]);
    // perror(error_message); // This prints the system error (e.g., "No such file or directory")
    handleError(error_message); // Use consistent error handling

    // Cleanup allocated memory if execvp fails
    for(int k=0; k < allocated_count; ++k) {
        free(allocated_paths[k]);
    }

    // Standard practice is for child to exit if exec fails.
    // The caller (e.g. bg or executeInFG) might waitpid and get this exit status.
    // exit(EXIT_FAILURE); // Or return an error code for parent to handle.
    // The original code returned 1. Let's stick to that for now.
    return 1;
}