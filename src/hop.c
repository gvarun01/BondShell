#include "../include/hop.h"
#include "../utils/error_handler.h" // For handleError
#include "../utils/path_utils.h"   // For pathCorrect
#include "../utils/string_utils.h" // For trimstr (if args need trimming)
#include <stdio.h>  // For printf
#include <string.h> // For strtok, strcpy, strcmp, strlen
#include <stdlib.h> // For malloc, free
#include <unistd.h> // For chdir, getcwd

void hop(char *arguments_string){ // Renamed str_input for clarity
    char current_dir_before_hop[MAX_PATH];
    if (getcwd(current_dir_before_hop, sizeof(current_dir_before_hop)) == NULL) {
        handleError("hop: getcwd failed to get current directory");
        // This is problematic as we can't set lastDir correctly.
        current_dir_before_hop[0] = '\0'; // Mark as invalid
    }

    if(arguments_string == NULL || strlen(arguments_string) == 0){ // "hop" with no arguments
        if(chdir(home) == -1){
            handleError("hop: chdir to home directory failed");
        } else {
            if(strlen(current_dir_before_hop) > 0) strcpy(lastDir, current_dir_before_hop);
            // No need to getcwd again if chdir was to 'home' and 'home' is known absolute.
            printf("%s\n", home);
        }
        fflush(stdout);
        return;
    }

    // Make a mutable copy for strtok
    char *args_copy = strdup(arguments_string);
    if (args_copy == NULL) {
        handleError("hop: strdup failed for arguments");
        return;
    }

    char *targets[MAX_HOP]; // Array to store pointers to target paths
    int num_targets = 0;    // Number of target paths found

    char *tokenizer_context;
    char *token = strtok_r(args_copy, " \t\n", &tokenizer_context);
    while (token != NULL && num_targets < MAX_HOP)
    {
        targets[num_targets] = strdup(token); // Duplicate each token
        if (targets[num_targets] == NULL) {
            handleError("hop: strdup failed for a target token");
            // Free previously allocated target tokens before returning
            for (int k=0; k<num_targets; ++k) free(targets[k]);
            free(args_copy);
            return;
        }
        num_targets++;
        token = strtok_r(NULL, " \t\n", &tokenizer_context);
    }
    free(args_copy); // Free the duplicated argument string

    if (num_targets == 0) { // Should not happen if arguments_string was not empty, but good check
        handleError("hop: No target paths specified.");
        return;
    }

    char subsequent_hop_current_dir[MAX_PATH]; // To store CWD for multi-hops correctly
    if(strlen(current_dir_before_hop) > 0) strcpy(subsequent_hop_current_dir, current_dir_before_hop);
    else subsequent_hop_current_dir[0] = '\0';


    for(int i = 0; i < num_targets; i++){
        char *path_to_hop_to = targets[i]; // This is already a duplicated string
        char *corrected_target_path = NULL; // Will hold path from pathCorrect or lastDir

        if(strcmp(path_to_hop_to, "-") == 0){
            if (strlen(lastDir) == 0) {
                handleError("hop: OLDPWD not set");
                free(path_to_hop_to); // Free the current target[i]
                continue; // Skip to next target
            }
            corrected_target_path = strdup(lastDir); // Use OLDPWD
            if (corrected_target_path == NULL) {
                handleError("hop: strdup failed for lastDir");
                free(path_to_hop_to);
                continue;
            }
            printf("%s\n", corrected_target_path); // POSIX `cd -` prints the new directory
        } else {
            corrected_target_path = pathCorrect(path_to_hop_to);
            if (corrected_target_path == NULL) {
                // pathCorrect already called handleError
                free(path_to_hop_to);
                continue;
            }
        }

        // Before changing directory, capture current one for next iteration's OLDPWD
        char dir_before_this_specific_chdir[MAX_PATH];
        if (getcwd(dir_before_this_specific_chdir, sizeof(dir_before_this_specific_chdir)) == NULL) {
            handleError("hop: getcwd failed before chdir attempt");
            dir_before_this_specific_chdir[0] = '\0';
        }


        if (chdir(corrected_target_path) == -1) {
            char error_msg[MAX_PATH + 50];
            snprintf(error_msg, sizeof(error_msg), "hop: chdir failed for '%s'", corrected_target_path);
            handleError(error_msg);
            // No change in directory, so lastDir should remain from previous successful hop (or initial)
        } else {
            // Successfully changed directory
            if(strlen(dir_before_this_specific_chdir) > 0) {
                 strcpy(lastDir, dir_before_this_specific_chdir); // Update OLDPWD
            }

            // Get and print the new current working directory
            if (getcwd(subsequent_hop_current_dir, sizeof(subsequent_hop_current_dir)) == NULL) {
                handleError("hop: getcwd failed after successful chdir");
                // Fallback to printing corrected_target_path, though it might not be absolute/clean
                printf("%s (getcwd failed)\n", corrected_target_path);
            } else {
                printf("%s\n", subsequent_hop_current_dir);
            }
        }

        free(corrected_target_path); // Free path from pathCorrect or strdup(lastDir)
        free(path_to_hop_to);      // Free target[i] from strdup(token)
    }

    fflush(stdout); // Ensure all output is displayed
}