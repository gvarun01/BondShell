#include "../include/log.h"
#include "../utils/error_handler.h" // For handleError
#include <stdio.h>  // For FILE ops, remove, rename, printf, perror
#include <string.h> // For strcpy, strncpy, strcmp, strtok, strlen
#include <stdlib.h> // For malloc, free
#include <unistd.h> // For open, read, write, lseek, close
#include <fcntl.h>  // For O_CREAT, O_RDWR, O_APPEND

// Initializes the command log, reading existing history if available.
CommandLog *initLog()
{
    CommandLog *log_instance = (CommandLog *)malloc(sizeof(CommandLog));
    if (log_instance == NULL) {
        handleError("log: Malloc failed for CommandLog structure.");
        return NULL;
    }

    log_instance->lineCount = 0;
    log_instance->prevCommand[0] = '\0'; // Initialize prevCommand to empty

    // Ensure logfile path (global) is initialized before this.
    if (strlen(logfile) == 0) {
        handleError("log: Logfile path not initialized.");
        free(log_instance);
        return NULL;
    }
    strcpy(log_instance->pathtohistory, logfile);

    // Open (or create) the history file.
    // Using mode 0600 for user read/write only.
    log_instance->fd = open(log_instance->pathtohistory, O_CREAT | O_RDWR | O_APPEND, 0600);
    if(log_instance->fd == -1)
    {
        char err_msg[MAX_PATH + 50];
        snprintf(err_msg, sizeof(err_msg), "log: Opening history file '%s' failed", log_instance->pathtohistory);
        perror(err_msg); // perror provides system error message
        free(log_instance);
        return NULL;
    }

    // Read existing history to populate lineCount and prevCommand.
    // This implementation reads the whole file into a buffer, then tokenizes.
    // For very large history files, a line-by-line approach might be more memory efficient.
    // MAX_HISTORY is 15, so 15 * MAX_COMMAND should be manageable.
    char file_content_buffer[MAX_HISTORY * MAX_COMMAND];
    ssize_t total_bytes_read = 0;
    ssize_t bytes_read_this_call;

    // Temporarily move file pointer to the beginning to read contents
    lseek(log_instance->fd, 0, SEEK_SET);

    while(total_bytes_read < sizeof(file_content_buffer) -1 &&
          (bytes_read_this_call = read(log_instance->fd, file_content_buffer + total_bytes_read, sizeof(file_content_buffer) - 1 - total_bytes_read)) > 0)
    {
        total_bytes_read += bytes_read_this_call;
    }
    file_content_buffer[total_bytes_read] = '\0'; // Null-terminate the buffer

    if(bytes_read_this_call == -1)
    {
        perror("log: Reading history file failed");
        // Continue with an empty history if read fails but file opened.
    }

    // Tokenize the buffer to count lines and get the last command
    char *tokenizer_context;
    char *line = strtok_r(file_content_buffer, "\n", &tokenizer_context);
    while(line != NULL)
    {
        log_instance->lineCount++;
        // Update prevCommand with the current line (last non-empty line will remain)
        // Ensure not to overflow prevCommand buffer.
        strncpy(log_instance->prevCommand, line, MAX_COMMAND - 1);
        log_instance->prevCommand[MAX_COMMAND - 1] = '\0';
        line = strtok_r(NULL, "\n", &tokenizer_context);
    }

    // Reset file pointer to the end for appending new commands
    lseek(log_instance->fd, 0, SEEK_END);
    return log_instance;
}

// Helper to get the last command from the history file directly.
// This avoids relying on the in-memory 'prevCommand' which might not be perfectly synced
// if multiple shell instances write to the same file (though not typical for this design).
static char *get_last_command_from_file(const char *history_filepath) {
    FILE *file = fopen(history_filepath, "r");
    if (file == NULL) {
        // perror("log: Failed to open history file for checking last command");
        return NULL; // Return NULL if file can't be opened
    }
    char last_read_line[MAX_COMMAND] = {0};
    char current_line_buffer[MAX_COMMAND];
    while (fgets(current_line_buffer, sizeof(current_line_buffer), file) != NULL) {
        // Keep overwriting last_read_line with the current line
        strncpy(last_read_line, current_line_buffer, MAX_COMMAND -1);
        last_read_line[MAX_COMMAND-1] = '\0';
    }
    fclose(file);

    // Remove trailing newline if present
    size_t len = strlen(last_read_line);
    if (len > 0 && last_read_line[len - 1] == '\n') {
        last_read_line[len - 1] = '\0';
    }

    if (strlen(last_read_line) > 0) {
        return strdup(last_read_line);
    }
    return NULL;
}


// Adds a command to the history log.
void addCommand(const char *command_to_add)
{
    if (command_to_add == NULL || strlen(command_to_add) == 0) return;
    if (history == NULL) {
        handleError("log: History not initialized, cannot add command.");
        return;
    }

    // Avoid adding consecutive duplicate commands
    if (strcmp(history->prevCommand, command_to_add) == 0)
    {
        return;
    }
    // Also check against the actual last command in file, in case prevCommand is stale
    char *last_in_file = get_last_command_from_file(history->pathtohistory);
    if(last_in_file != NULL) {
        if (strcmp(last_in_file, command_to_add) == 0) {
            free(last_in_file);
            return;
        }
        free(last_in_file);
    }


    if(history->lineCount < MAX_HISTORY)
    {
        // Simply append if history is not full
        if (write(history->fd, command_to_add, strlen(command_to_add)) == -1 ||
            write(history->fd, "\n", 1) == -1) {
            perror("log: Failed to write command to history file");
            // Attempt to close and reopen to fix fd issues? Or just error out.
        } else {
            history->lineCount++;
        }
    }
    else // History is full, need to remove the oldest command
    {
        // This involves reading all but the first line into a temp file,
        // then renaming temp file to original history file.
        FILE *hist_file_ptr = fopen(history->pathtohistory, "r");
        if (!hist_file_ptr)
        {
            perror("log: Could not open history file for rotation");
            return;
        }

        // Ensure tempfile_path (global) is set
        if (strlen(tempfile_path) == 0) {
            handleError("log: Temporary file path for history rotation not set.");
            fclose(hist_file_ptr);
            return;
        }
        FILE *temp_hist_file_ptr = fopen(tempfile_path, "w");
        if (!temp_hist_file_ptr)
        {
            perror("log: Could not open temporary history file for rotation");
            fclose(hist_file_ptr);
            return;
        }

        char line_buffer[MAX_COMMAND];
        int lines_skipped = 0;
        // Skip the first line (oldest command)
        if(fgets(line_buffer, sizeof(line_buffer), hist_file_ptr) != NULL) {
            lines_skipped++;
        }
        // Write remaining lines to temp file
        while(fgets(line_buffer, sizeof(line_buffer), hist_file_ptr)) {
            fputs(line_buffer, temp_hist_file_ptr);
        }
        fclose(hist_file_ptr);

        // Append the new command to the temp file
        fprintf(temp_hist_file_ptr, "%s\n", command_to_add);
        fclose(temp_hist_file_ptr);

        // Replace old history file with temp file
        if (remove(history->pathtohistory) != 0) {
            perror("log: Failed to remove old history file during rotation");
            // Attempt to remove temp file to clean up
            remove(tempfile_path);
            return;
        }
        if (rename(tempfile_path, history->pathtohistory) != 0) {
            perror("log: Failed to rename temp history file during rotation");
            // This is problematic: old history is gone, new one couldn't be named.
            // Try to recover by writing current command to a new history file?
            // For now, just error.
            return;
        }

        // Re-open the main history file descriptor for future appends
        close(history->fd); // Close old fd
        history->fd = open(history->pathtohistory, O_CREAT | O_RDWR | O_APPEND, 0600);
        if(history->fd == -1) {
            perror("log: Failed to reopen history file after rotation");
            // lineCount might be off now. This is a bad state.
        }
        // lineCount remains MAX_HISTORY
    }

    // Update in-memory prevCommand
    strncpy(history->prevCommand, command_to_add, MAX_COMMAND - 1);
    history->prevCommand[MAX_COMMAND-1] = '\0';
}


// This function is not used by the shell logic directly.
// If it were for removing the *last* added command (like an undo), its logic would be different.
// The current implementation seems to try to remove the Nth command, but its file manipulation is complex.
// Given it's not used, and MAX_HISTORY is small, this function could be removed or simplified if needed.
// For now, commenting out its content as it's complex and might have issues.
void removeCommand()
{
    /*
    if(history == NULL || history->lineCount == 0) return;

    // This function as previously written was complex.
    // A simpler approach if removing the last command:
    // 1. Read all but the last line into a temp file.
    // 2. Replace original with temp.
    // 3. Update lineCount and prevCommand.
    // However, this is not a typical shell feature. `log purge` is more common.
    handleError("log: removeCommand function is currently not supported in this refactoring.");
    */
}

// Prints the command history to stdout.
void printLog(){
    if (history == NULL || history->lineCount == 0)
    {
        printf("No history available.\n");
        return;
    }

    FILE *file = fopen(history->pathtohistory, "r");
    if (file == NULL)
    {
        char err_msg[MAX_PATH + 50];
        snprintf(err_msg, sizeof(err_msg), "log: Failed to open history file '%s' for printing", history->pathtohistory);
        perror(err_msg);
        return;
    }

    char line_buffer[MAX_COMMAND];
    int command_number = 1;
    while (fgets(line_buffer, sizeof(line_buffer), file))
    {
        // Remove trailing newline for cleaner printing if present
        line_buffer[strcspn(line_buffer, "\n")] = '\0';
        printf("%d. %s\n", command_number++, line_buffer);
    }
    fclose(file);
}

// Clears the command history.
void purgeLog(){
    if (history == NULL) {
        handleError("log: History not initialized, cannot purge.");
        return;
    }
    // Close current fd, truncate file by opening with "w", then reopen for append.
    close(history->fd);
    FILE *file = fopen(history->pathtohistory, "w"); // Opens for writing, truncates existing file
    if (!file)
    {
        char err_msg[MAX_PATH + 50];
        snprintf(err_msg, sizeof(err_msg), "log: Failed to open history file '%s' for purging", history->pathtohistory);
        perror(err_msg);
        // Attempt to reopen in append mode anyway
        history->fd = open(history->pathtohistory, O_CREAT | O_RDWR | O_APPEND, 0600);
        return;
    }
    fclose(file); // Close after truncation

    // Reopen the main fd for future appends
    history->fd = open(history->pathtohistory, O_CREAT | O_RDWR | O_APPEND, 0600);
    if (history->fd == -1) {
         perror("log: Failed to reopen history file after purge");
    }

    history->lineCount = 0;
    history->prevCommand[0] = '\0';
    printf("Command history purged.\n");
}

// Retrieves a command from history by its (1-based) index from the end.
// e.g., index 1 is the last command, index 2 is second to last.
char *executeLog(int index_from_end){
    if (history == NULL || index_from_end <= 0 || index_from_end > history->lineCount)
    {
        handleError("log execute: Invalid index or no history.");
        return NULL;
    }

    FILE *file = fopen(history->pathtohistory, "r");
    if (!file)
    {
        perror("log execute: Failed to open history file");
        return NULL;
    }

    char line_buffer[MAX_COMMAND];
    // Calculate the 0-based line number to retrieve (from start of file)
    int target_line_number = history->lineCount - index_from_end;
    int current_line = 0;
    char *found_command = NULL;

    while (fgets(line_buffer, sizeof(line_buffer), file))
    {
        if(current_line == target_line_number){
            // Remove trailing newline
            line_buffer[strcspn(line_buffer, "\n")] = '\0';
            found_command = strdup(line_buffer);
            if (found_command == NULL) {
                handleError("log execute: strdup failed for command from log");
            }
            break;
        }
        current_line++;
    }

    fclose(file);
    if (found_command == NULL && current_line <= target_line_number) {
        // Should not happen if index_from_end <= history->lineCount
        handleError("log execute: Command not found at calculated index (logic error?).");
    }
    return found_command; // Caller must free this string
}

// Frees resources used by the command log.
void freeLog(){
    if (history != NULL) {
        if (history->fd != -1) {
            close(history->fd);
        }
        free(history);
        history = NULL; // Set global to NULL
    }
}