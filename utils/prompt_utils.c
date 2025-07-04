#include "prompt_utils.h"
#include "path_utils.h" // For relativePath
#include <stdio.h>      // For printf
#include <string.h>     // For strlen
#include <unistd.h>     // For usleep, getcwd

// Displays the introductory welcome message for the shell
void intro()
{
    // ANSI escape code to hide cursor
    printf(STYLE_BOLD COLOR_MAGENTA "\033[?25l\n");
    printf("\t\t\t            ____                  _   ____  _          _ _\n");
    usleep(100000); // Shorter delay for faster startup
    printf("\t\t\t           | __ )  ___  _ __   __| | / ___|| |__   ___| | |\n");
    usleep(100000);
    printf("\t\t\t           |  _ \\ / _ \\| '_ \\ / _` | \\___ \\| '_ \\ / _ \\ | |\n");
    usleep(100000);
    printf("\t\t\t           | |_) | (_) | | | | (_| |  ___) | | | |  __/ | |\n");
    usleep(100000);
    printf("\t\t\t           |____/ \\___/|_| |_|\\__,_| |____/|_| |_|\\___|_|_|\n");
    usleep(100000);
    printf("\n");
    // ANSI escape code to show cursor
    printf("\033[?25h" COLOR_RESET);

    printf(COLOR_BLUE STYLE_BOLD "\nWelcome to BOND Shell 🛡️ 👾,\n");
    printf("\ta simple Interactive shell written in C.\n" COLOR_RESET);
    printf("Type ");
    printf(COLOR_YELLOW "help" COLOR_RESET);
    printf(" to list all the available commands.\n");
    printf("Type ");
    printf(COLOR_YELLOW "exit" COLOR_RESET);
    printf(" to exit the shell.\n");
    printf("Crafted with " COLOR_RED STYLE_BOLD "❤️" COLOR_RESET " by Varun Gupta.\n\n");
}

// Displays the shell prompt
// Renamed from 'promt' to 'display_prompt'
void display_prompt(char *prompt_message_suffix) {
    char current_working_dir[MAX_PATH];
    if(getcwd(current_working_dir, sizeof(current_working_dir)) == NULL){
        handleError("getcwd failed while preparing prompt");
        // Fallback prompt
        printf(COLOR_RED "error_getting_path> " COLOR_RESET);
        return;
    }

    char *relative_path_to_display;
    relative_path_to_display = relativePath(current_working_dir);
    if (relative_path_to_display == NULL) {
        // Fallback if relativePath fails
        printf("<" STYLE_BOLD COLOR_RED "%s" COLOR_RESET  STYLE_BOLD "@" COLOR_GREEN "%s" COLOR_RESET STYLE_BOLD ":" COLOR_MAGENTA "%s" COLOR_RESET, username, sysname, "error_path");
    } else {
        printf("<" STYLE_BOLD COLOR_RED "%s" COLOR_RESET  STYLE_BOLD "@" COLOR_GREEN "%s" COLOR_RESET STYLE_BOLD ":" COLOR_MAGENTA "%s" COLOR_RESET, username, sysname, relative_path_to_display);
        free(relative_path_to_display); // Free memory allocated by relativePath
    }

    if(prompt_message_suffix != NULL && strlen(prompt_message_suffix) > 0){
        printf(COLOR_YELLOW " %s " COLOR_RESET, prompt_message_suffix);
    }
    printf("> ");
    fflush(stdout); // Ensure prompt is displayed immediately
}
