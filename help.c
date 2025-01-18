#include "help.h"

void displayHelp() {
    printf("\n");
    printf(STYLE_BOLD COLOR_YELLOW "Bond Shell Help Page:\n" COLOR_RESET);

    printf("Welcome to the Bond Shell!\nWhenenver there is a problem" COLOR_BLUE" Bond " COLOR_RESET "is here to here to help You 😊\nHere are the commands you can use:\n\n");

    printf(STYLE_BOLD "1. exit 🚪\n" COLOR_RESET);
    printf("   - Exits the shell. Goodbye! 👋\n\n");

    printf(STYLE_BOLD "2. hop <path> 🏠\n" COLOR_RESET);
    printf("   - Changes the current directory.\n");
    printf("   - Supports '.', '..', '~', and '-' flags.\n");
    printf("   - Works with both absolute and relative paths.\n\n");

    printf(STYLE_BOLD "3. reveal <flags> <path> 🔍\n" COLOR_RESET);
    printf("   - Lists files and directories in lexicographic order.\n");
    printf("   - Flags:\n");
    printf("     - **-a**: Show all files (including hidden ones) 🌟\n");
    printf("     - **-l**: Long format (detailed information) 📋\n");
    printf("   - Supports '.', '..', '~', and '-' symbols.\n\n");

    printf(STYLE_BOLD "4. log 📜\n" COLOR_RESET);
    printf("   - Stores and displays the last 15 commands.\n");
    printf("   - **log purge**: Clears the log 🗑️\n");
    printf("   - **log execute <index>**: Executes a command from the log ▶️\n\n");

    printf(STYLE_BOLD "5. System Commands 💻\n" COLOR_RESET);
    printf("   - Execute system commands like `emacs`, `gedit`, etc.\n");
    printf("   - Supports both foreground and background processes.\n");
    printf("   - For searching about any cmd use cmd --help\n\n");


    printf(STYLE_BOLD "6. proclore [pid] 🕵️‍♂️\n" COLOR_RESET);
    printf("   - Displays information about a process.\n");
    printf("   - Includes: PID, status, process group, virtual memory, executable path.\n\n");

    printf(STYLE_BOLD "7. seek <flags> <search> <target_directory> 🔎\n" COLOR_RESET);
    printf("   - Searches for a file or directory in the specified directory.\n");
    printf("   - Flags:\n");
    printf("     - **-d**: Directories only 📂\n");
    printf("     - **-f**: Files only 📄\n");
    printf("     - **-e**: Execute if single match found 🚀\n");
    printf("   - Supports '.', '..', '~', and '-' symbols.\n\n");

    printf(STYLE_BOLD "8. cd <path> 🌐\n" COLOR_RESET);
    printf("   - Changes the current directory.\n");
    printf("   - Supports '~' to navigate to your home directory 🏠.\n\n");

    printf(STYLE_BOLD "9. help ❓\n" COLOR_RESET);
    printf("   - Displays this help page.\n");
    printf("\n");
}
