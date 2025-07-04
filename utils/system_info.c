#include "system_info.h"
#include <unistd.h> // For getlogin_r, gethostname, getcwd
#include <stdio.h>  // For printf (used in error cases)
#include <stdlib.h> // For exit
#include <string.h> // For strcpy, strcat

// Initializes shell environment variables like username, system name, home directory, etc.
// Renamed from 'initialise' to 'initialize_shell_environment'
void initialize_shell_environment()
{
    if (getlogin_r(username, MAX_NAME) != 0)
    {
        perror("Error getting username"); // Use perror for system call errors
        // Decide if this is a fatal error. For now, let's try to continue.
        strncpy(username, "user", MAX_NAME -1); // Fallback username
        username[MAX_NAME-1] = '\0';
    }

    if (gethostname(sysname, MAX_NAME) != 0)
    {
        perror("Error getting system name");
        strncpy(sysname, "host", MAX_NAME-1); // Fallback system name
        sysname[MAX_NAME-1] = '\0';
    }

    if (getcwd(wrkdirectory, MAX_PATH) == NULL)
    {
        perror("Error getting initial working directory");
        // This is more critical, as 'home' directory depends on it.
        // Fallback to a default or exit. For a shell, exiting might be safer.
        fprintf(stderr, "Fatal: Could not determine current working directory at startup.\n");
        exit(EXIT_FAILURE);
    }

    // Set home directory to the initial working directory
    if (strlen(wrkdirectory) < MAX_PATH) {
        strcpy(home, wrkdirectory);
    } else {
        fprintf(stderr, "Fatal: Initial working directory path is too long.\n");
        exit(EXIT_FAILURE);
    }


    // Initialize paths for log file and temporary file relative to home directory
    // Ensure paths don't overflow
    if (strlen(home) + strlen("/history.txt") < MAX_PATH) {
        strcpy(logfile, home);
        strcat(logfile, "/history.txt");
    } else {
        fprintf(stderr, "Warning: Could not construct logfile path (too long).\n");
        // Fallback or disable logging
        logfile[0] = '\0';
    }

    if (strlen(home) + strlen("/temp.txt") < MAX_PATH) {
        strcpy(tempfile_path, home);
        strcat(tempfile_path, "/temp.txt");
    } else {
        fprintf(stderr, "Warning: Could not construct tempfile_path (too long).\n");
        tempfile_path[0] = '\0';
    }

    // Initialize lastDir to an empty string or a sensible default.
    // Empty is fine, indicates no 'cd -' history initially.
    lastDir[0] = '\0';
}
