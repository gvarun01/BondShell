#include "../include/neonate.h"
#include "../utils/error_handler.h" // For handleError
#include "../utils/string_utils.h"  // For trimstr
#include <stdio.h>      // For printf, snprintf, getchar, perror, fflush
#include <stdlib.h>     // For atoi, opendir, readdir, closedir, exit, malloc, free
#include <string.h>     // For strstr, strtok, strcmp, strcpy
#include <dirent.h>     // For DIR, struct dirent, opendir, readdir, closedir
#include <sys/stat.h>   // For stat, struct stat
#include <time.h>       // For time_t
#include <unistd.h>     // For STDIN_FILENO, usleep
#include <termios.h>    // For struct termios, tcgetattr, tcsetattr, TCSANOW, ICANON, ECHO
#include <signal.h>     // For sigset_t, sigemptyset, sigaddset, sigprocmask
#include <sys/select.h> // For fd_set, FD_ZERO, FD_SET, select, struct timeval, FD_ISSET (for kbhit)

// Finds the PID of the most recently created process by checking /proc directory entries' ctime.
static int get_most_recent_pid() // Made static
{
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL)
    {
        perror("neonate: opendir /proc failed");
        return -1;
    }

    struct dirent *dir_entry;
    pid_t most_recent_pid = -1; // Use pid_t for PIDs
    time_t most_recent_time = 0;

    while ((dir_entry = readdir(proc_dir)) != NULL)
    {
        if (dir_entry->d_type == DT_DIR) // Check if it's a directory
        {
            pid_t pid = atoi(dir_entry->d_name); // Convert directory name to PID
            if (pid > 0) // Valid PIDs are positive integers
            {
                char stat_path[MAX_PATH]; // Assuming MAX_PATH is defined and sufficient
                snprintf(stat_path, sizeof(stat_path), "/proc/%d", pid);

                struct stat process_stat_info;
                if (stat(stat_path, &process_stat_info) == 0)
                {
                    // Using st_mtime (modification time of status file) or st_ctime (inode change time)
                    // st_ctime is often related to metadata changes. For process start, mtime of /proc/[pid] dir is good.
                    if (process_stat_info.st_mtime > most_recent_time)
                    {
                        most_recent_time = process_stat_info.st_mtime;
                        most_recent_pid = pid;
                    }
                }
            }
        }
    }

    closedir(proc_dir);
    return most_recent_pid;
}

// Non-blocking keyboard hit detection.
static int kbhit() // Made static
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0; // No wait time
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); // Check stdin
    // select checks if any of the fds in the read set are ready for reading
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds); // Return true if stdin has data
}

void neonate(char *arguments_str) // Renamed subcom_input
{
    if (arguments_str == NULL) {
        handleError("neonate: Missing arguments. Usage: neonate -n <seconds>");
        return;
    }

    char *args_copy = strdup(arguments_str);
    if (args_copy == NULL) {
        handleError("neonate: strdup failed for arguments.");
        return;
    }

    char *tokenizer_context;
    char *token_n_flag = strtok_r(args_copy, " \t\n", &tokenizer_context);

    if (token_n_flag == NULL || strcmp(token_n_flag, "-n") != 0)
    {
        handleError("neonate: Invalid arguments. Missing or incorrect '-n' flag. Usage: neonate -n <seconds>");
        free(args_copy);
        return;
    }

    char *time_arg_str = strtok_r(NULL, " \t\n", &tokenizer_context);
    if (time_arg_str == NULL)
    {
        handleError("neonate: Missing time argument after -n. Usage: neonate -n <seconds>");
        free(args_copy);
        return;
    }

    char *extra_arg = strtok_r(NULL, " \t\n", &tokenizer_context);
    if (extra_arg != NULL)
    {
        handleError("neonate: Too many arguments. Usage: neonate -n <seconds>");
        free(args_copy);
        return;
    }

    int interval_seconds = atoi(time_arg_str);
    if (interval_seconds <= 0 && strcmp(time_arg_str, "0") != 0) // atoi returns 0 for non-numeric or invalid
    {
        handleError("neonate: Invalid time interval provided. Must be a non-negative integer.");
        free(args_copy);
        return;
    }
    free(args_copy); // Arguments parsed, free the copy


    // Terminal setup for non-canonical mode (no line buffering, no echo)
    struct termios old_term_settings, new_term_settings;
    tcgetattr(STDIN_FILENO, &old_term_settings);
    new_term_settings = old_term_settings;
    new_term_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term_settings);

    // Block SIGINT and SIGTSTP to prevent interruption of neonate itself by Ctrl+C/Ctrl+Z
    // These signals should terminate the foreground process neonate is observing, not neonate.
    // However, the problem description implies 'x' is the only exit.
    // This might be for the shell's main loop, not neonate. Re-evaluating if needed.
    // For now, let's assume 'x' is the primary exit and signals are handled by the shell.
    // If neonate itself needs to be impervious, it might need to ignore them or have specific handlers.
    // The original sigprocmask was to block them *during* neonate's loop.

    printf(COLOR_MAGENTA "Neonate mode activated. PID updates every %d second(s). Press " COLOR_RESET COLOR_YELLOW "'x'" COLOR_MAGENTA " to exit.\n" COLOR_RESET, interval_seconds);
    fflush(stdout);

    bool exit_neonate = false;
    while (!exit_neonate)
    {
        pid_t recent_pid = get_most_recent_pid();
        if (recent_pid == -1)
        {
            // Error already printed by get_most_recent_pid
            break; // Exit loop on error
        }
        printf("%d\n", recent_pid);
        fflush(stdout);

        // Wait for interval_seconds, checking for 'x' key press every millisecond.
        // This makes the 'x' key press responsive.
        if (interval_seconds == 0) { // Special case: if interval is 0, check kbhit continuously (busy wait)
             if (kbhit()) {
                if (getchar() == 'x') {
                    exit_neonate = true;
                }
            }
            usleep(10000); // Small sleep to prevent 100% CPU usage if interval is 0
        } else {
            for (long long i = 0; i < interval_seconds * 1000; i++) // Loop for interval_seconds * 1000 milliseconds
            {
                if (kbhit())
                {
                    if (getchar() == 'x')
                    {
                        exit_neonate = true;
                        break; // Exit inner loop
                    }
                }
                usleep(1000); // Sleep for 1 millisecond
            }
        }
    }

    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term_settings);
    printf(COLOR_MAGENTA "\nNeonate mode deactivated.\n" COLOR_RESET);
}