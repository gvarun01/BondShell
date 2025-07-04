#include "../include/parser.h"
#include "../utils/string_utils.h"  // For trimstr
#include "../utils/shell_utils.h"   // For exitCheck
#include "../utils/error_handler.h" // For handleError
#include <stdio.h>  // For perror (though handleError is preferred)
#include <stdlib.h> // For malloc, free, exit, EXIT_FAILURE
#include <string.h> // For strlen, strcpy, strstr, strtok_r
#include <unistd.h> // For dup, pipe, close, STDIN_FILENO, STDOUT_FILENO

// Parses and executes commands, handling semicolons and pipes.
void parser(char *command_input_str) // Renamed command_input
{
    if (command_input_str == NULL) {
        handleError("parser: NULL input string received.");
        return;
    }

    // Make a mutable copy for strtok_r, as it modifies the string.
    char *input_copy_for_semicolon = strdup(command_input_str);
    if (input_copy_for_semicolon == NULL)
    {
        handleError("parser: strdup failed for input_copy_for_semicolon.");
        // perror("strdup"); // Can also use perror for system/library call errors
        return; // Cannot proceed without a mutable copy
    }

    // Add command to history, unless it's a "log" command to avoid recursive history addition
    // or if it's empty after trimming.
    char temp_trimmed_input[MAX_COMMAND];
    strncpy(temp_trimmed_input, command_input_str, MAX_COMMAND-1);
    temp_trimmed_input[MAX_COMMAND-1] = '\0';
    trimstr(temp_trimmed_input);

    if (strlen(temp_trimmed_input) > 0 && strstr(temp_trimmed_input, "log") != temp_trimmed_input) // Check if "log" is at the beginning
    {
        addCommand(temp_trimmed_input); // Add the original, trimmed command
    }


    char *semicolon_tokenizer_context;
    char *semicolon_command = strtok_r(input_copy_for_semicolon, ";", &semicolon_tokenizer_context);

    while (semicolon_command != NULL)
    {
        char current_semicolon_segment[MAX_COMMAND];
        strncpy(current_semicolon_segment, semicolon_command, MAX_COMMAND-1);
        current_semicolon_segment[MAX_COMMAND-1] = '\0';
        trimstr(current_semicolon_segment);

        if (strlen(current_semicolon_segment) == 0) { // Skip empty commands from ";;" or trailing ";"
            semicolon_command = strtok_r(NULL, ";", &semicolon_tokenizer_context);
            continue;
        }

        // exitCheck needs to be called for each part separated by ';'
        // However, if "exit" is part of a pipe, its behavior might be complex.
        // Standard shells typically exit immediately.
        exitCheck(current_semicolon_segment);

        // Check for invalid pipe usage, e.g., "cmd |" or "| cmd" or "cmd || cmd"
        if (current_semicolon_segment[0] == '|' || current_semicolon_segment[strlen(current_semicolon_segment) - 1] == '|')
        {
            handleError("parser: Invalid use of pipes (leading/trailing/double).");
            semicolon_command = strtok_r(NULL, ";", &semicolon_tokenizer_context);
            continue; // Skip this malformed segment
        }

        // Save original stdin and stdout for restoration after pipes
        int original_stdin_fd = dup(STDIN_FILENO);
        int original_stdout_fd = dup(STDOUT_FILENO);
        if(original_stdin_fd == -1 || original_stdout_fd == -1) {
            handleError("parser: dup failed to save original stdio FDs.");
            if(original_stdin_fd != -1) close(original_stdin_fd);
            if(original_stdout_fd != -1) close(original_stdout_fd);
            // This is a critical state, maybe abort this semicolon segment.
            semicolon_command = strtok_r(NULL, ";", &semicolon_tokenizer_context);
            continue;
        }


        // Make a copy of the current semicolon_command segment for pipe tokenization
        char *pipe_segment_copy = strdup(current_semicolon_segment);
        if (pipe_segment_copy == NULL) {
            handleError("parser: strdup failed for pipe_segment_copy.");
            close(original_stdin_fd);
            close(original_stdout_fd);
            semicolon_command = strtok_r(NULL, ";", &semicolon_tokenizer_context);
            continue;
        }

        char *pipe_tokenizer_context;
        char *pipe_command_token = strtok_r(pipe_segment_copy, "|", &pipe_tokenizer_context);

        char *commands_in_pipe[MAX_PIPES]; // Store pointers to tokens
        int num_pipe_commands = 0;

        while (pipe_command_token != NULL && num_pipe_commands < MAX_PIPES)
        {
            // Each token needs to be duplicated if executeCommand modifies its input.
            // executeCommand currenty makes its own copy.
            commands_in_pipe[num_pipe_commands] = strdup(pipe_command_token);
            if(commands_in_pipe[num_pipe_commands] == NULL) {
                handleError("parser: strdup failed for pipe command token.");
                // Free previously allocated tokens
                for(int k=0; k<num_pipe_commands; ++k) free(commands_in_pipe[k]);
                num_pipe_commands = 0; // Reset count
                break; // Break from while loop for tokens
            }
            trimstr(commands_in_pipe[num_pipe_commands]); // Trim each part of the pipe
            num_pipe_commands++;
            pipe_command_token = strtok_r(NULL, "|", &pipe_tokenizer_context);
        }
        free(pipe_segment_copy); // Free the copy used for tokenizing pipes

        if (num_pipe_commands == 0 && strlen(current_semicolon_segment) > 0) { // e.g. if segment was just "   "
             // This case should be handled by the initial trim of current_semicolon_segment
        } else if (num_pipe_commands == 1) // Single command, no pipes
        {
            executeCommand(commands_in_pipe[0]);
            free(commands_in_pipe[0]); // Free the strdup'd command
        }
        else if (num_pipe_commands > 1) // Piped commands
        {
            // Backgrounding a whole pipeline (e.g. "cmd1 | cmd2 &") needs careful thought.
            // The current executeCommand handles '&' at the end of its *entire* input.
            // If current_semicolon_segment ends with '&', it's complex.
            // For now, let's assume pipes don't interact with the sequential '&' parser in executeCommand.
            // This means "a | b &" would try to background 'b', not the whole pipe.
            // A robust shell often forks once for the whole pipeline.

            int pipe_fds[2]; // For a single pipe between two commands
            int input_fd_for_next_command = original_stdin_fd; // First command reads from original stdin

            for (int i = 0; i < num_pipe_commands; i++)
            {
                if (strlen(commands_in_pipe[i]) == 0) { // Skip empty commands from "cmd1 || cmd2"
                    if (i > 0) close(input_fd_for_next_command); // close read end of previous pipe if any
                    input_fd_for_next_command = STDIN_FILENO; // Reset for safety, though this segment is skipped
                    continue;
                }

                // If not the first command, stdin comes from the previous command's pipe output
                if (i > 0) {
                    if (dup2(input_fd_for_next_command, STDIN_FILENO) == -1) {
                        handleError("parser: dup2 failed for pipe input redirection.");
                        close(input_fd_for_next_command); // close this fd
                        // Attempt to restore stdio and break pipe execution
                        dup2(original_stdin_fd, STDIN_FILENO);
                        dup2(original_stdout_fd, STDOUT_FILENO);
                        break;
                    }
                    close(input_fd_for_next_command); // Close the read end of the pipe, child will have its copy
                }

                // If not the last command, stdout goes to a new pipe
                if (i < num_pipe_commands - 1)
                {
                    if (pipe(pipe_fds) == -1)
                    {
                        handleError("parser: pipe creation failed.");
                        // Attempt to restore stdio and break pipe execution
                        if (i > 0) dup2(original_stdin_fd, STDIN_FILENO); // Restore if changed
                        dup2(original_stdout_fd, STDOUT_FILENO);
                        break;
                    }
                    if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) { // Redirect stdout to pipe write end
                        handleError("parser: dup2 failed for pipe output redirection.");
                        close(pipe_fds[0]); close(pipe_fds[1]);
                        if (i > 0) dup2(original_stdin_fd, STDIN_FILENO);
                        dup2(original_stdout_fd, STDOUT_FILENO);
                        break;
                    }
                    close(pipe_fds[1]); // Close the write end in parent, child will have its copy
                    input_fd_for_next_command = pipe_fds[0]; // Next command's input is read end of this pipe
                }
                else // Last command, stdout is original stdout
                {
                    if(dup2(original_stdout_fd, STDOUT_FILENO) == -1) {
                        handleError("parser: dup2 failed to restore original stdout for last pipe command.");
                        // This is problematic.
                    }
                }

                executeCommand(commands_in_pipe[i]); // This will fork for external commands
                // After executeCommand, the parent shell's STDIN/STDOUT should be restored for the next pipe segment or command.
                // This is handled by restoring original_stdin_fd/original_stdout_fd after the loop.
            }

            // Restore original stdin and stdout for the shell after pipeline execution
            if(dup2(original_stdin_fd, STDIN_FILENO) == -1) handleError("parser: dup2 failed to restore original stdin post-pipe.");
            if(dup2(original_stdout_fd, STDOUT_FILENO) == -1) handleError("parser: dup2 failed to restore original stdout post-pipe.");
        }

        // Free duplicated command tokens for the current pipe sequence
        for (int i = 0; i < num_pipe_commands; i++)
        {
            free(commands_in_pipe[i]);
        }

        // Close the duplicated original stdio FDs
        close(original_stdin_fd);
        close(original_stdout_fd);

        semicolon_command = strtok_r(NULL, ";", &semicolon_tokenizer_context);
    }
    free(input_copy_for_semicolon); // Free the initial copy of the full input string
}