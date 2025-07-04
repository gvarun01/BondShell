#include "path_utils.h"
#include <string.h> // For strcmp, strncmp, strcpy, strcat, strrchr
#include <stdlib.h> // For malloc
#include <unistd.h> // For getcwd

// Corrects path based on special characters like ~ . ..
char *pathCorrect(char *path)
{
    char *newPath = (char *)malloc(2 * MAX_PATH);
    if (newPath == NULL) {
        handleError("Memory allocation failed in pathCorrect");
        return NULL; // Should not proceed if allocation fails
    }

    if (strcmp(path, "~") == 0)
    {
        strcpy(newPath, home);
    }
    else if (strncmp(path, "~/", 2) == 0) // More specific check for home directory paths like ~/Documents
    {
        strcpy(newPath, home);
        strcat(newPath, path + 1); // Append the rest of the path after ~
    }
    else if (strcmp(path, ".") == 0)
    {
        if (getcwd(newPath, 2 * MAX_PATH) == NULL) // Use allocated size
        {
            handleError("getcwd failed in pathCorrect");
            free(newPath); // Free allocated memory before returning
            return NULL;
        }
    }
    else if (strcmp(path, "..") == 0)
    {
        char currentPath[MAX_PATH];
        if (getcwd(currentPath, MAX_PATH) == NULL)
        {
            handleError("getcwd failed in pathCorrect");
            free(newPath);
            return NULL;
        }
        strcpy(newPath, currentPath);
        char *last_slash = strrchr(newPath, '/');
        if (last_slash != NULL) {
            if (last_slash == newPath) // Root directory case like /
            {
                if (newPath[1] != '\0') { // Ensure it's not just "/"
                     newPath[1] = '\0';
                }
            } else {
                *last_slash = '\0'; // Truncate to parent directory
            }
        } else { // Should not happen for absolute paths from getcwd
            // If it's somehow a relative path without slashes, this logic might be insufficient
            // However, getcwd returns absolute paths.
        }
        if (strlen(newPath) == 0 && strcmp(currentPath, "/") == 0) { // if original was root
             strcpy(newPath, "/");
        }


    }
    else // Absolute or relative path
    {
        if (path[0] != '/') // Relative path
        {
            if (getcwd(newPath, MAX_PATH) == NULL) // MAX_PATH here, then strcat potentially overflows
            {
                handleError("getcwd failed in pathCorrect");
                free(newPath);
                return NULL;
            }
            // Ensure enough space before strcat
            if (strlen(newPath) + strlen(path) + 2 <= 2 * MAX_PATH) {
                 strcat(newPath, "/");
                 strcat(newPath, path);
            } else {
                handleError("Path too long in pathCorrect");
                free(newPath);
                return NULL;
            }
        }
        else // Absolute path
        {
            // Ensure path fits into newPath
            if (strlen(path) < 2 * MAX_PATH) {
                strcpy(newPath, path);
            } else {
                handleError("Path too long in pathCorrect");
                free(newPath);
                return NULL;
            }
        }
    }
    return newPath;
}

// Converts absolute path to a path relative to home if applicable
char *relativePath(char *path)
{
    char *newPath = (char *)malloc(2 * MAX_PATH);
    if (newPath == NULL) {
        handleError("Memory allocation failed in relativePath");
        return NULL;
    }

    if (strncmp(path, home, strlen(home)) == 0)
    {
        // Ensure enough space for "~" and the rest of the path
        if (strlen(path) - strlen(home) + 2 <= 2 * MAX_PATH) {
            strcpy(newPath, "~");
            strcat(newPath, path + strlen(home));
        } else {
            handleError("Path too long for relative conversion");
            // Fallback to original path or handle error differently
            strcpy(newPath, path);
        }
    }
    else
    {
        if (strlen(path) < 2 * MAX_PATH) {
            strcpy(newPath, path);
        } else {
            handleError("Path too long in relativePath");
            // Fallback or error
            // For now, let's just truncate, though this is not ideal
            strncpy(newPath, path, 2*MAX_PATH -1);
            newPath[2*MAX_PATH -1] = '\0';
        }
    }
    return newPath;
}
