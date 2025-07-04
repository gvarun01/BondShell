#include "shell_utils.h"
#include "string_utils.h" // For trimstr
#include "../include/signal.h" // For killAllProcesses (assuming it's there)
#include <stdio.h>    // For printf
#include <stdlib.h>   // For exit, malloc, free
#include <string.h>   // For strcmp, strcpy

// Checks if the command is "exit" and terminates the shell if so.
// It also calls killAllProcesses before exiting.
void exitCheck(char *command)
{
    // It's important that trimstr is available and works correctly.
    // Make a copy for trimstr and strcmp to avoid modifying the input `command`
    // if it's used by the caller afterwards.
    char *command_copy = strdup(command);
    if (command_copy == NULL) {
        handleError("strdup failed in exitCheck");
        return; // Or handle more gracefully
    }

    trimstr(command_copy); // trimstr is now in string_utils.c

    if (strcmp(command_copy, "exit") == 0)
    {
        printf(COLOR_YELLOW "Exiting shell.....\n" COLOR_RESET);
        killAllProcesses(); // This function needs to be accessible, e.g. from signal.h
        free(command_copy);
        exit(EXIT_SUCCESS);
    }
    free(command_copy);
}

// Swaps the content of two character arrays (strings).
// Assumes arrays are large enough (MAX_PATH).
// Renamed from swap_c for clarity.
void swap_char_arrays(char *arr1, char *arr2)
{
    char temp_buffer[MAX_PATH]; // Using a stack buffer, ensure it's large enough

    // Check for NULL pointers to prevent crashes
    if (arr1 == NULL || arr2 == NULL) {
        handleError("NULL pointer passed to swap_char_arrays");
        return;
    }

    // Ensure that MAX_PATH is indeed the size of these arrays or use a dynamic approach
    // For now, assuming they are fixed-size arrays of at least MAX_PATH.
    strncpy(temp_buffer, arr1, MAX_PATH -1);
    temp_buffer[MAX_PATH-1] = '\0';

    strncpy(arr1, arr2, MAX_PATH -1);
    arr1[MAX_PATH-1] = '\0';

    strncpy(arr2, temp_buffer, MAX_PATH-1);
    arr2[MAX_PATH-1] = '\0';
}
