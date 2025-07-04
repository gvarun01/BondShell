#include "global.h"
// #include "../include/signal.h" // This will be included by specific modules needing killAllProcesses, like shell_utils.c

// These global variables are accessed by many parts of the shell.
// Consideration for future: encapsulate them into a shell_state struct.
char username[MAX_NAME];
char sysname[MAX_NAME];
char wrkdirectory[MAX_PATH]; // Stores initial working directory
char home[MAX_PATH];         // Stores home directory path, typically same as wrkdirectory at start

int executionTime = 0; // For tracking command execution time for prompt
char executedCommand[MAX_COMMAND] = ""; // Stores the last executed command name for prompt
char promtExec[MAX_COMMAND] = ""; // Suffix for the prompt, e.g., showing execution time
char lastDir[MAX_PATH] = "";      // Stores the last working directory for 'cd -' functionality
char logfile[MAX_PATH] = "";      // Path to the history file
char tempfile_path[MAX_PATH] = ""; // Path to a temporary file used by history management

// Global definitions for background processes
int bgcount = 0;
struct bgprocess *bgs = NULL; // Array of background processes

// Global definitions for foreground process tracking
int fgPid = -1; // PID of the current foreground process, -1 if none
char *fgCommand = NULL; // Command string of the current foreground process

// Global for command history log
struct CommandLog *history = NULL;

// Global for shell input buffer
char input[MAX_COMMAND] = "";

// Globals for I/O redirection state
char *input_file = NULL;  // Path to input redirection file
char *output_file = NULL; // Path to output redirection file
int stdout_copy_fd = -1;  // Saved file descriptor for stdout

// Functions that were here have been moved to more specific utility files:
// - handleError -> utils/error_handler.c
// - initialise -> utils/system_info.c (renamed to initialize_shell_environment)
// - trimstr -> utils/string_utils.c
// - pathCorrect -> utils/path_utils.c
// - relativePath -> utils/path_utils.c
// - getcmd -> utils/string_utils.c
// - intro -> utils/prompt_utils.c
// - swap_c -> utils/shell_utils.c (renamed to swap_char_arrays)
// - exitCheck -> utils/shell_utils.c
// - promt -> utils/prompt_utils.c (renamed to display_prompt)