#ifndef GLOBAL_H
#define GLOBAL_H

// _POSIX_C_SOURCE is now defined in Makefile CFLAGS

// Standard Library Includes - Essential for most parts of the shell
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // For signal handling constants and functions like kill, signal
#include <errno.h>  // For errno

// Other common includes that were in the original global.h
// These might be moved to specific modules if not universally needed
#include <pwd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Project specific headers should NOT be included in this global system header provider.
// Modules should include specific project headers they need.
// For example, utils/shell_utils.c includes "../include/signal.h" for killAllProcesses.
                                // if exitCheck calls killAllProcesses.

// Color and Style Definitions
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_BRIGHT_RED "\x1b[91m"
#define COLOR_ERROR COLOR_YELLOW // Specific color for error messages
#define STYLE_BOLD "\x1b[1m"

// Max Size Definitions
#define MAX_COMMAND 4096
#define MAX_PATH 4096
#define MAX_NAME 256
#define MAX_HISTORY 15
#define MAX_HOP 100 // Used in hop.c for number of targets
#define MAX_BG_PROCESSES 1000
#define MAX_PIPES 100

// Struct Definitions for global data structures
struct bgprocess {
    pid_t id;
    char comm[MAX_COMMAND];
};

typedef struct CommandLog {
    int lineCount;
    char prevCommand[MAX_COMMAND]; // Stores the immediately previous command to avoid duplicates
    int fd;                        // File descriptor for the history file
    char pathtohistory[2 * MAX_PATH]; // Full path to the history file
} CommandLog;

// External Declarations for Global Variables (defined in utils/global.c)
// These are widely used across the shell.
extern char username[MAX_NAME];
extern char sysname[MAX_NAME];
extern char wrkdirectory[MAX_PATH]; // Initial working directory at shell start
extern char home[MAX_PATH];         // Home directory of the user

extern int executionTime;           // Time taken by the last foreground command
extern char executedCommand[MAX_COMMAND]; // Name of the last command for prompt display
extern char promtExec[MAX_COMMAND]; // String to append to prompt (e.g., exec time)
extern char lastDir[MAX_PATH];      // Previous working directory for 'cd -'
extern char logfile[MAX_PATH];      // Path to command history file
extern char tempfile_path[MAX_PATH]; // Path to temporary file for history management

extern int bgcount;                 // Count of current background processes
extern struct bgprocess *bgs;       // Array of background process info

extern int fgPid;                   // PID of the current foreground process (-1 if none)
extern char *fgCommand;             // Command string of the current foreground process

extern struct CommandLog *history;  // Pointer to the command history structure

extern char input[MAX_COMMAND];     // Buffer for user input

extern char *input_file;            // Filename for input redirection
extern char *output_file;           // Filename for output redirection
extern int stdout_copy_fd;          // Saved STDOUT file descriptor for restoring after redirection


// Function declarations for utilities that were originally in global.h and are now moved.
// These are removed as they are now in their respective utility headers.
// void initialise(); -> moved to utils/system_info.h (initialize_shell_environment)
// void trimstr(char *str); -> moved to utils/string_utils.h
// void handleError(const char *message); -> moved to utils/error_handler.h
// char *pathCorrect(char *path); -> moved to utils/path_utils.h
// char *getcmd(char *cmd); -> moved to utils/string_utils.h
// char *relativePath(char *path); -> moved to utils/path_utils.h
// void intro(); -> moved to utils/prompt_utils.h
// void swap_c(char *a, char *b); -> moved to utils/shell_utils.h (swap_char_arrays)
// void exitCheck(char *command); -> moved to utils/shell_utils.h

#endif // GLOBAL_H