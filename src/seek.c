#include "../include/seek.h"
#include "../utils/error_handler.h" // For handleError
#include "../utils/path_utils.h"   // For pathCorrect
#include "../utils/string_utils.h" // For trimstr
#include <stdio.h>      // For snprintf, printf, FILE, fopen, fgets, fclose, perror
#include <string.h>     // For strtok_r, strncmp, strcpy, strlen, strcspn, strcmp
#include <stdlib.h>     // For free, strdup (if used for tokens)
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir
#include <sys/stat.h>   // For struct stat, stat, S_ISDIR
#include <unistd.h>     // For chdir (if -e flag for directory)

// Recursive helper function for seek
// Renamed parameters for clarity
static void seek_recursive_helper(const char *search_target, const char *current_search_path,
                                  bool search_dirs_only, bool search_files_only,
                                  int *match_count, char *first_match_fullpath,
                                  const char *relative_display_path_prefix)
{
    DIR *dir_stream = opendir(current_search_path);
    if (dir_stream == NULL) {
        char err_msg[MAX_PATH + 50];
        snprintf(err_msg, sizeof(err_msg), "seek: Cannot open directory '%s'", current_search_path);
        // perror(err_msg); // Printing this for every unreadable dir might be too verbose.
                          // handleError might be better if only one message is desired.
        // handleError(err_msg); // Or make it conditional.
        return;
    }

    struct dirent *dir_entry;
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
            continue; // Skip . and ..
        }

        char full_entry_path[MAX_PATH * 2]; // Path for stat and recursive call
        snprintf(full_entry_path, sizeof(full_entry_path), "%s/%s", current_search_path, dir_entry->d_name);

        char relative_display_path[MAX_PATH * 2]; // Path for printing
        snprintf(relative_display_path, sizeof(relative_display_path), "%s/%s",
                 relative_display_path_prefix, dir_entry->d_name);


        struct stat entry_stat;
        if (stat(full_entry_path, &entry_stat) == -1) { // Use stat; lstat if symlink behavior needs to be different for search
            // Could print error for each failed stat, or ignore.
            // perror_msg for full_entry_path
            continue;
        }

        bool is_match = (strncmp(dir_entry->d_name, search_target, strlen(search_target)) == 0);

        if (S_ISDIR(entry_stat.st_mode)) { // It's a directory
            if (is_match && !search_files_only) { // Match if dir and (searching all or dirs_only)
                printf(COLOR_BLUE "%s\n" COLOR_RESET, relative_display_path);
                (*match_count)++;
                if (*match_count == 1) { // Store first match path
                    strncpy(first_match_fullpath, full_entry_path, MAX_PATH * 4 -1);
                    first_match_fullpath[MAX_PATH*4-1] = '\0';
                }
            }
            // Recursively search in this directory
            seek_recursive_helper(search_target, full_entry_path, search_dirs_only, search_files_only,
                                  match_count, first_match_fullpath, relative_display_path);
        } else { // It's a file (or other type, not a directory)
            if (is_match && !search_dirs_only) { // Match if file and (searching all or files_only)
                printf(COLOR_GREEN "%s\n" COLOR_RESET, relative_display_path);
                (*match_count)++;
                 if (*match_count == 1) { // Store first match path
                    strncpy(first_match_fullpath, full_entry_path, MAX_PATH * 4 -1);
                     first_match_fullpath[MAX_PATH*4-1] = '\0';
                }
            }
        }
    }
    closedir(dir_stream);
}

// Prints the contents of a file
static void print_file_contents(const char *file_path) { // Made static, const char*
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        char err_msg[MAX_PATH + 50];
        snprintf(err_msg, sizeof(err_msg), "seek: Failed to open file '%s' for printing contents", file_path);
        perror(err_msg);
        return;
    }

    char line_buffer[MAX_COMMAND]; // Assuming MAX_COMMAND is large enough for a line
    while (fgets(line_buffer, sizeof(line_buffer), file)) {
        printf("%s", line_buffer);
    }
    // printf("\n"); // Add a newline if files might not end with one, or remove if lines have it.
    fclose(file);
}

void seek(char *arguments_str) // Renamed str
{
    if (arguments_str == NULL || strlen(arguments_str) == 0) {
        handleError("seek: Missing arguments. Usage: seek [-d|-f] [-e] <target> [directory]");
        return;
    }

    bool search_dirs_only = false;  // -d flag
    bool search_files_only = false; // -f flag
    bool execute_if_one_match = false; // -e flag
    char search_target_name[MAX_NAME] = "";
    char base_search_dir[MAX_PATH] = "."; // Default to current directory

    // Parse arguments
    char *args_copy = strdup(arguments_str);
    if(args_copy == NULL) {
        handleError("seek: strdup failed for arguments.");
        return;
    }
    trimstr(args_copy); // Trim overall string first

    char *tokenizer_context;
    char *token = strtok_r(args_copy, " \t\n", &tokenizer_context);
    int non_flag_args_count = 0;

    while (token != NULL) {
        if (strcmp(token, "-d") == 0) {
            search_dirs_only = true;
        } else if (strcmp(token, "-f") == 0) {
            search_files_only = true;
        } else if (strcmp(token, "-e") == 0) {
            execute_if_one_match = true;
        } else { // Not a flag, must be target or path
            if (non_flag_args_count == 0) { // First non-flag is the target
                strncpy(search_target_name, token, MAX_NAME - 1);
                search_target_name[MAX_NAME - 1] = '\0';
            } else if (non_flag_args_count == 1) { // Second non-flag is the directory
                strncpy(base_search_dir, token, MAX_PATH - 1);
                base_search_dir[MAX_PATH - 1] = '\0';
            } else {
                handleError("seek: Too many non-flag arguments.");
                free(args_copy);
                return;
            }
            non_flag_args_count++;
        }
        token = strtok_r(NULL, " \t\n", &tokenizer_context);
    }
    free(args_copy);

    if (strlen(search_target_name) == 0) {
        handleError("seek: Search target name missing.");
        return;
    }
    if (search_dirs_only && search_files_only) {
        handleError("seek: Flags -d and -f are mutually exclusive.");
        return;
    }

    // Remove potential trailing newline from target and path (though strtok usually handles this)
    search_target_name[strcspn(search_target_name, "\n")] = '\0';
    base_search_dir[strcspn(base_search_dir, "\n")] = '\0';


    char *resolved_base_search_path = pathCorrect(base_search_dir);
    if (resolved_base_search_path == NULL) {
        // pathCorrect calls handleError
        return;
    }

    int match_count = 0;
    // Buffer for first_match_fullpath needs to be large enough for any possible path.
    // MAX_PATH * 4 was used, maybe MAX_PATH * 2 is more realistic if relative_display_path_prefix is also MAX_PATH.
    char first_match_fullpath[MAX_PATH * 2] = "";

    // The initial relative_display_path_prefix should be the user-provided path, or "."
    char initial_relative_prefix[MAX_PATH];
    strncpy(initial_relative_prefix, base_search_dir, MAX_PATH-1); // Use original base_search_dir for display
    initial_relative_prefix[MAX_PATH-1] = '\0';


    seek_recursive_helper(search_target_name, resolved_base_search_path,
                          search_dirs_only, search_files_only,
                          &match_count, first_match_fullpath, initial_relative_prefix);

    if (match_count == 0) {
        // No specific message for no matches, as per typical `find` behavior (just no output)
        // However, original code had "seek: no matches found", so keeping it for consistency:
        printf("No match found.\n");
    } else if (execute_if_one_match && match_count == 1) {
        struct stat match_stat;
        if (stat(first_match_fullpath, &match_stat) == 0) {
            if (S_ISDIR(match_stat.st_mode)) {
                if (chdir(first_match_fullpath) == 0) {
                    printf("Changed directory to: %s\n", first_match_fullpath);
                } else {
                    char err_msg[MAX_PATH + 60];
                    snprintf(err_msg, sizeof(err_msg), "seek: chdir to '%s' failed", first_match_fullpath);
                    perror(err_msg);
                }
            } else if (S_ISREG(match_stat.st_mode)) { // Check if it's a regular file before printing
                // Check for execute permission if we were to execute it.
                // For now, just print contents as per original behavior.
                print_file_contents(first_match_fullpath);
            } else {
                handleError("seek: Matched item is not a directory or regular file, cannot execute default action.");
            }
        } else {
            char err_msg[MAX_PATH + 50];
            snprintf(err_msg, sizeof(err_msg), "seek: stat failed for matched item '%s'", first_match_fullpath);
            perror(err_msg);
        }
    }

    free(resolved_base_search_path); // Free path from pathCorrect
}