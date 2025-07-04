#include "../include/reveal.h"
#include "../utils/error_handler.h" // For handleError
#include "../utils/path_utils.h"   // For pathCorrect
#include <stdio.h>      // For printf, snprintf, perror
#include <string.h>     // For strtok_r, strncmp, strcpy, strlen, strcmp, strdup
#include <stdlib.h>     // For qsort, free
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir
#include <sys/stat.h>   // For struct stat, lstat, S_ISDIR, S_IXUSR, etc.
#include <pwd.h>        // For struct passwd, getpwuid
#include <grp.h>        // For struct group, getgrgid
#include <time.h>       // For strftime, localtime, time_t
#include <ctype.h>      // For tolower

// Helper function to print file/directory name with appropriate color
static void print_colored_name(const char *name, mode_t mode) { // Made static, const char*
    if (S_ISDIR(mode)) {
        printf(COLOR_BLUE "%s" COLOR_RESET, name);
    } else if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) { // Check for execute permissions
        printf(COLOR_GREEN "%s" COLOR_RESET, name);
    } else {
        printf("%s", name); // Default color (usually white or terminal default)
    }
}

// Comparison function for qsort to sort directory entries alphabetically (case-insensitive for primary sort key).
// Special handling for "." and ".." to appear first.
static int compare_dir_entries(const void *a_ptr, const void *b_ptr) { // Made static
    const char *name_a = *(const char **)a_ptr;
    const char *name_b = *(const char **)b_ptr;

    // "." should always come before ".." and other files
    if (strcmp(name_a, ".") == 0) return -1; // a comes first
    if (strcmp(name_b, ".") == 0) return 1;  // b comes first

    // ".." should come after "." but before other files
    if (strcmp(name_a, "..") == 0) return -1;
    if (strcmp(name_b, "..") == 0) return 1;

    // For other files, case-insensitive comparison
    // Skip leading '.' for hidden files if not sorting them specially
    const char *cmp_a = (name_a[0] == '.') ? name_a + 1 : name_a;
    const char *cmp_b = (name_b[0] == '.') ? name_b + 1 : name_b;

    int result = strcasecmp(cmp_a, cmp_b); // strcasecmp for case-insensitive
    if (result == 0) {
        // If case-insensitively equal, use case-sensitive for tie-breaking
        return strcmp(name_a, name_b);
    }
    return result;
}

// Prints file permissions in rwxrwxrwx format.
static void print_file_permissions(mode_t mode) { // Made static
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

// Prints details for a single file or directory.
static void print_single_entry_details(const char *base_path, const char *entry_name, bool show_hidden, bool long_format) { // Made static
    if (!show_hidden && entry_name[0] == '.') { // Skip hidden files if not requested
        return;
    }

    char full_entry_path[MAX_PATH * 2]; // Increased buffer for safety
    snprintf(full_entry_path, sizeof(full_entry_path), "%s/%s", base_path, entry_name);

    struct stat file_stat_info;
    if (lstat(full_entry_path, &file_stat_info) < 0) { // Use lstat to handle symbolic links correctly
        char err_msg[MAX_PATH + 50];
        snprintf(err_msg, sizeof(err_msg), "reveal: lstat failed for %s", full_entry_path);
        perror(err_msg); // perror provides more info than just handleError
        return;
    }


    if (long_format) {
        print_file_permissions(file_stat_info.st_mode);

        printf(" %2ld", (long)file_stat_info.st_nlink); // Number of hard links

        struct passwd *owner_info = getpwuid(file_stat_info.st_uid);
        printf(" %-8s", (owner_info) ? owner_info->pw_name : "UNKNOWN");

        struct group *group_info = getgrgid(file_stat_info.st_gid);
        printf(" %-8s", (group_info) ? group_info->gr_name : "UNKNOWN");

        printf(" %8ld", (long)file_stat_info.st_size); // File size in bytes

        char time_buffer[100];
        strftime(time_buffer, sizeof(time_buffer), "%b %d %H:%M", localtime(&file_stat_info.st_mtime));
        printf(" %s ", time_buffer);

        print_colored_name(entry_name, file_stat_info.st_mode);

        // If it's a symbolic link, show where it points
        if (S_ISLNK(file_stat_info.st_mode)) {
            char link_target_buffer[MAX_PATH];
            ssize_t len = readlink(full_entry_path, link_target_buffer, sizeof(link_target_buffer) - 1);
            if (len != -1) {
                link_target_buffer[len] = '\0';
                printf(" -> %s", link_target_buffer);
            }
        }
        printf("\n");
    } else {
        print_colored_name(entry_name, file_stat_info.st_mode);
        printf("\n");
    }
}

void reveal(char *arguments_str) // Renamed str
{
    bool show_hidden = false;
    bool long_format = false;
    char path_to_reveal[MAX_PATH + 1] = ""; // Directory to list, default current

    if (arguments_str != NULL && strlen(arguments_str) > 0) {
        char *args_copy = strdup(arguments_str);
        if (args_copy == NULL) {
            handleError("reveal: strdup failed for arguments.");
            return;
        }
        char *tokenizer_context;
        char *token = strtok_r(args_copy, " \t\n", &tokenizer_context);

        while (token != NULL) {
            if (token[0] == '-') { // It's a flag
                if (strchr(token, 'a') != NULL) show_hidden = true;
                if (strchr(token, 'l') != NULL) long_format = true;
                // Add checks for invalid flags if desired
            } else { // It's a path argument
                if (strlen(path_to_reveal) == 0) { // First non-flag token is the path
                    strncpy(path_to_reveal, token, MAX_PATH);
                    path_to_reveal[MAX_PATH] = '\0';
                } else {
                    handleError("reveal: Multiple path arguments provided. Only one is allowed.");
                    free(args_copy);
                    return;
                }
            }
            token = strtok_r(NULL, " \t\n", &tokenizer_context);
        }
        free(args_copy);
    }

    if (strlen(path_to_reveal) == 0) { // If no path was given, default to "."
        strcpy(path_to_reveal, ".");
    }

    char *corrected_base_path = pathCorrect(path_to_reveal);
    if (corrected_base_path == NULL) {
        // pathCorrect calls handleError, so just return
        return;
    }

    DIR *directory_stream = opendir(corrected_base_path);
    if (directory_stream == NULL) {
        char err_msg[MAX_PATH + 50];
        snprintf(err_msg, sizeof(err_msg), "reveal: Cannot open directory '%s'", corrected_base_path);
        perror(err_msg);
        free(corrected_base_path);
        return;
    }

    struct dirent *dir_entry;
    char *dir_entries_list[2048]; // Max entries to sort, adjust if needed
    int num_entries = 0;
    long total_blocks = 0; // For 'total' in long format

    while ((dir_entry = readdir(directory_stream)) != NULL && num_entries < 2048) {
        if (!show_hidden && dir_entry->d_name[0] == '.') {
            continue; // Skip hidden files unless -a
        }
        dir_entries_list[num_entries] = strdup(dir_entry->d_name);
        if (dir_entries_list[num_entries] == NULL) {
             handleError("reveal: strdup failed for directory entry name.");
             // Free previously strdup'd entries
             for(int k=0; k<num_entries; ++k) free(dir_entries_list[k]);
             closedir(directory_stream);
             free(corrected_base_path);
             return;
        }

        if (long_format) { // Collect block sizes for 'total'
            char temp_full_path[MAX_PATH * 2];
            snprintf(temp_full_path, sizeof(temp_full_path), "%s/%s", corrected_base_path, dir_entry->d_name);
            struct stat entry_stat;
            if (stat(temp_full_path, &entry_stat) == 0) { // Use stat, not lstat, for block count of target for links
                total_blocks += entry_stat.st_blocks;
            }
        }
        num_entries++;
    }
    closedir(directory_stream);

    qsort(dir_entries_list, num_entries, sizeof(char *), compare_dir_entries);

    if (long_format && num_entries > 0) { // Print total block size (usually in 512-byte blocks or 1K blocks)
        printf("total %ld\n", total_blocks / 2); // st_blocks is often in 512B units, ls shows 1K units.
    }

    for (int i = 0; i < num_entries; i++) {
        print_single_entry_details(corrected_base_path, dir_entries_list[i], show_hidden, long_format);
        free(dir_entries_list[i]); // Free the strdup'd name
    }

    free(corrected_base_path); // Free path from pathCorrect
}