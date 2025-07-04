#include "../include/IO_redirection.h"
#include "../utils/error_handler.h" // For handleError
#include "../utils/path_utils.h"   // For pathCorrect
#include "../utils/string_utils.h" // For trimstr (used implicitly by pathCorrect sometimes or directly)
#include <stdio.h>  // For freopen, stderr, printf (for debugging, should be removed)
#include <string.h> // For strcpy, strstr, strtok
#include <stdlib.h> // For malloc, free

char* handleIO(char *command_got){

    // It's safer to check if command_got is NULL
    if (command_got == NULL) {
        handleError("NULL command passed to handleIO");
        return NULL;
    }

    char *command = strdup(command_got); // Use strdup for safety
    if (command == NULL) {
        handleError("strdup failed in handleIO for command");
        return NULL;
    }

    char *modified_command_str = strdup(command_got); // Will be tokenized to extract command part
    if (modified_command_str == NULL) {
        handleError("strdup failed in handleIO for modified_command_str");
        free(command);
        return NULL;
    }

    char *tempPt = NULL;

    if (strstr(command, "<<") != NULL)
    {
        handleError("Unsupported I/O redirection");
        return NULL;
    }
    tempPt = strstr(command, "<");
    if (tempPt != NULL)
    {
        char *temp = (char *)malloc((strlen(command) + 1) * sizeof(char));
        if (temp == NULL)
        {
            handleError("Error allocating memory");
            return NULL;
        }
        strcpy(temp, command);

        char *x;
        char *part1 __attribute__((unused)) = strtok_r(temp, "<", &x); // Mark as unused
        char *part2 = tempPt + 1;

        // need to take input file from part2
        if (part2 == NULL)
        {
            handleError("Invalid I/O redirection");
            free(temp);
            return NULL;
        }
        trimstr(part1);
        trimstr(part2);

        input_file = strtok(part2, " \t\n");
        if (input_file == NULL)
        {
            handleError("Invalid I/O redirection");
            free(temp);
            return NULL;
        }
        trimstr(input_file);
        input_file = pathCorrect(input_file);
        printf("input file: %s\n", input_file);
        if (freopen(input_file, "r", stdin) == NULL)
        {
            handleError("Input File Not Found");
            free(temp);
            return NULL;
        }
        free(temp);
    }

    tempPt = strstr(command, ">");
    if (tempPt != NULL)
    {
        // two possibilities > or >>
        char *tempPt2 = strstr(command, ">>");
        if (tempPt2 != NULL)
        {
            char *temp = (char *)malloc((strlen(command) + 1) * sizeof(char));
            if (temp == NULL)
            {
                handleError("Error allocating memory");
                return NULL;
            }
            strcpy(temp, command);

            char *x;
            char *part1 __attribute__((unused)) = strtok_r(temp, ">>", &x); // Mark as unused

            char *part2 = tempPt2 + 2;
            if (part2 == NULL)
            {
                handleError("Invalid I/O redirection");
                free(temp);
                return NULL;
            }
            trimstr(part2);

            output_file = strtok(part2, " \t\n");

            if (output_file == NULL)
            {
                handleError("Invalid I/O redirection");
                free(temp);
                return NULL;
            }
            trimstr(output_file);
            output_file = pathCorrect(output_file);

            if (freopen(output_file, "a", stdout) == NULL)
            {
                handleError("Output File Not Found");
                free(temp);
                return NULL;
            }
            free(temp);
        }
        else
        {
            char *temp = (char *)malloc((strlen(command) + 1) * sizeof(char));
            if (temp == NULL)
            {
                handleError("Error allocating memory");
                return NULL;
            }
            strcpy(temp, command);

            char *x;
            char *part1 __attribute__((unused)) = strtok_r(temp, ">", &x); // Mark as unused
            char *part2 = tempPt + 1;
            if (part2 == NULL)
            {
                handleError("Invalid I/O redirection");
                free(temp);
                return NULL;
            }
            trimstr(part2);
        
            output_file = strtok(part2, " \t\n");

            if (output_file == NULL)
            {
                handleError("Invalid I/O redirection");
                free(temp);
                return NULL;
            }
            trimstr(output_file);
            output_file = pathCorrect(output_file);

            if (freopen(output_file, "w", stdout) == NULL)
            {
                handleError("Output File Not Found");
                free(temp);
                return NULL;
            }
            free(temp);
        }
        if(!tempPt2)
            free(tempPt2);
    }
    // The original command string (command_got or its copy 'command') has been modified by strtok_r earlier.
    // We need to extract the actual command part, excluding redirection clauses.
    // modified_command_str holds an original copy of command_got.

    char *final_command_part = strtok(modified_command_str, "><"); // Tokenize based on redirection operators

    if (final_command_part != NULL) {
        trimstr(final_command_part); // Clean up the extracted command part
        char *result = strdup(final_command_part); // Duplicate it for returning
        if(result == NULL){
            handleError("strdup failed for final_command_part in handleIO");
            // Fallthrough to free allocated memory
        }
        free(command); // Free the copy of command_got made at the beginning
        free(modified_command_str); // Free the second copy used for tokenizing command part
        return result; // Return the isolated command string
    } else {
        // This case implies the command was empty or only redirection symbols
        handleError("No command found after parsing I/O redirection");
        free(command);
        free(modified_command_str);
        return NULL;
    }
    // tempPt does not need to be freed as it points into 'command' string, which is freed.
}

void resetIO(){
    // Restore stdin, stdout, stderr to the terminal
    // This is crucial after redirection to ensure shell behaves correctly for subsequent commands.
    if(freopen("/dev/tty", "r", stdin) == NULL){
        handleError("Error restoring stdin to /dev/tty");
        // Potentially critical error, shell might misbehave
    }
    if(dup2(stdout_copy_fd, fileno(stdout)) == -1){ // Use the saved original stdout
        handleError("Error restoring stdout using dup2");
    }
    // Standard error is often not redirected by shells unless explicitly done (2>),
    // but resetting it to /dev/tty is a safe measure if it might have been.
    if(freopen("/dev/tty", "w", stderr) == NULL){
        handleError("Error restoring stderr to /dev/tty");
    }
    // Clear any error flags on stdin, stdout, stderr
    clearerr(stdin);
    clearerr(stdout);
    clearerr(stderr);
}