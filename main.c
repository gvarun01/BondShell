#include "utils/global.h"       // For global variables, structs, MAX_*, colors
#include "include/execute.h"    // For executeCommand, parser (indirectly if parser calls execute)
#include "include/config.h"     // For load_config, find_function, replace_alias, execute_function
#include "include/parser.h"     // For parser
#include "include/animation.h"  // For animation_open_close
#include "include/signal.h"     // For signal handlers like handleCtrlC, handleCtrlZ, killAllProcesses
#include "include/log.h"        // For initLog
#include "include/bg.h"         // For checkbg
// New utility headers
#include "utils/system_info.h"   // For initialize_shell_environment
#include "utils/prompt_utils.h"  // For intro, display_prompt
#include "utils/string_utils.h"  // For trimstr
#include "utils/shell_utils.h"   // For exitCheck
#include "utils/error_handler.h" // For handleError


// global variables initialization is now handled by their definitions in utils/global.c
// No need for separate extern declarations here if global.h is included.

int main()
{
    // Setup signal handlers
    // It's good practice to check the return value of signal()
    if (signal(SIGINT, handleCtrlC) == SIG_ERR) {
        handleError("Failed to set SIGINT handler");
        // Consider if this is fatal
    }
    if (signal(SIGTSTP, handleCtrlZ) == SIG_ERR) {
        handleError("Failed to set SIGTSTP handler");
        // Consider if this is fatal
    }

    printf(COLOR_MAGENTA "\033[?25l"); // hide the cursor
    animation_open_close(0); // Assuming this is a startup animation
    printf("\033[?25h" COLOR_RESET); // show the cursor

    // Memory allocation for global variables that are pointers
    // These should be allocated after initialize_shell_environment if they depend on it,
    // or if their sizes are dynamic.
    // For now, assuming MAX_BG_PROCESSES, MAX_COMMAND, MAX_PATH are compile-time constants.
    bgs = (struct bgprocess *)malloc(sizeof(struct bgprocess) * MAX_BG_PROCESSES);
    fgCommand = (char *)malloc(sizeof(char) * MAX_COMMAND);
    input_file = (char *)malloc(sizeof(char) * MAX_PATH); // Used by IO_redirection
    output_file = (char *)malloc(sizeof(char) * MAX_PATH); // Used by IO_redirection

    if (bgs == NULL || fgCommand == NULL || input_file == NULL || output_file == NULL)
    {
        handleError("Memory allocation failed for core structures");
        exit(EXIT_FAILURE); // Critical failure
    }

    // stdout_copy_fd is essential for restoring stdout after redirection (e.g. in bg.c)
    stdout_copy_fd = dup(fileno(stdout));
    if (stdout_copy_fd == -1)
    {
        handleError("Failed to duplicate stdout file descriptor");
        // This could be critical if redirection is used extensively and needs reset
        exit(EXIT_FAILURE);
    }

    initialize_shell_environment(); // Initialize username, sysname, home, logfile paths etc.
    history = initLog(); // Initialize command history (reads from logfile)
    if (history == NULL) {
        handleError("Failed to initialize command history");
        // Decide if this is fatal or if the shell can run without history
    }

    intro(); // Display welcome message
    load_config(".myshrc"); // Load aliases and functions

    while (1)
    {
        setbuf(stdout, NULL); // Disable buffering for stdout for interactive use
        fgPid = -1; // Reset foreground process ID for each new command cycle

        checkbg(); // Check for completed background processes

        display_prompt(promtExec); // Display the shell prompt; promtExec may contain exec time
        strcpy(promtExec, "");    // Clear prompt suffix for the next command
        fflush(stdout); // Ensure prompt is displayed

        if (fgets(input, MAX_COMMAND, stdin) == NULL) // Read user input
        {
            // fgets returns NULL on EOF (e.g., Ctrl+D) or error
            fflush(stdout); // Ensure any pending output is flushed
            killAllProcesses(); // Clean up any running child processes
            printf(COLOR_YELLOW "\nLogging out...\n" COLOR_RESET); // User-friendly exit message
            break; // Exit the main loop, leading to shell termination
        }

        if (strcmp(input, "\n") == 0) // Empty input, just show prompt again
        {
            continue;
        }
        input[strcspn(input, "\n")] = '\0'; // Remove trailing newline

        trimstr(input); // Remove leading/trailing whitespace

        // Before executing, check for aliases.
        // replace_alias modifies 'input' in place.
        char command_for_alias_replacement[MAX_COMMAND];
        strcpy(command_for_alias_replacement, input);
        replace_alias(command_for_alias_replacement); //This function modifies its argument
        strcpy(input, command_for_alias_replacement);


        // exitCheck should be called after alias replacement,
        // in case 'exit' itself is an alias or part of a function.
        exitCheck(input); // Terminates shell if input is "exit"

        // Make a copy of the input for tokenization, as strtok modifies the string
        char *command_copy = strdup(input);
        if (command_copy == NULL) {
            handleError("strdup failed for command copy");
            continue;
        }

        char *first_token = strtok(command_copy, " \t\n");
        char *remaining_args = strtok(NULL, ""); // Get the rest of the string as arguments for functions

        if (first_token != NULL && find_function(first_token) != NULL)
        {
            // If it's a user-defined function, execute it
            // Note: execute_function needs to be robust.
            // It might need the original 'input' or a parsed argument list.
            // The current find_function/execute_function seems to re-parse.
            execute_function(first_token, remaining_args);
        }
        else
        {
            // If not a function, parse and execute as a normal command or series of commands
            parser(input); // parser handles pipes, multiple commands separated by ';', etc.
        }

        fflush(stdout); // Ensure all output from command execution is displayed
        free(command_copy); // Free the duplicated command string
    }

    // Cleanup before exiting shell
    if (bgs != NULL) free(bgs);
    if (fgCommand != NULL) free(fgCommand);
    if (input_file != NULL) free(input_file);
    if (output_file != NULL) free(output_file);
    if (history != NULL) {
        // freeLog(history); // Assuming a function to free history resources
    }
    if (stdout_copy_fd != -1) close(stdout_copy_fd);


    return 0;
}