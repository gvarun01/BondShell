#include "seek.h"

void helper(char *target, char *resolved_path, int d, int f, int *found, char *single, char *relative)
{
    DIR *dir = opendir(resolved_path);
    if (dir == NULL)
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char full_path[2 * MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", resolved_path, entry->d_name);

        struct stat st;
        stat(full_path, &st);
        char new_relative[2 * MAX_PATH];
        snprintf(new_relative, sizeof(new_relative), "%s/%s", relative, entry->d_name);

        if (S_ISDIR(st.st_mode))
        {
            if ((d || (!d && !f)) && strncmp(entry->d_name, target, strlen(target)) == 0)
            {
                // printf("path - %s full - %s\n",path, full_path);
                printf(COLOR_BLUE "%s\n" COLOR_RESET, new_relative);
                *found = 1;
                strncpy(single, full_path, 4 * MAX_PATH - 1);
                single[4 * MAX_PATH - 1] = '\0';
            }

            if (strncmp(entry->d_name, ".", 1) == 0 || strncmp(entry->d_name, "..", 2) == 0)
            {
                continue;
            }
            helper(target, full_path, d, f, found, single, new_relative);
        }
        else
        {
            if ((f || (!d && !f)) && strncmp(entry->d_name, target, strlen(target)) == 0)
            {
                printf(COLOR_GREEN "%s\n" COLOR_RESET, new_relative);
                *found = 1;
                strncpy(single, full_path, 4 * MAX_PATH - 1);
                single[4 * MAX_PATH - 1] = '\0';
            }
        }
    }
    closedir(dir);
}

void printfilecontents(char *filePath){
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        handleError("fopen");
        return;
    }

    char line[MAX_COMMAND];
    while (fgets(line, sizeof(line), file))
    {
        printf("%s", line);
    }
    printf("\n");
    fclose(file);
}

void seek(char *str)
{
    if (str == NULL)
    {
        handleError("seek: missing argument");
        return;
    }

    int d = 0, f = 0, e = 0;
    char target[MAX_NAME] = "";
    char path[MAX_PATH] = "";
    trimstr(str);
    char *token;

    char *x;
    token = strtok_r(str, " \t", &x);
    while (token != NULL)
    {
        if(strncmp(token, "-", 1) == 0){
            for(int i = 1; i < strlen(token); i++){
                if(token[i] == '-'){
                    continue;
                }
                else if(token[i] == 'd'){
                    d = 1;
                }
                else if(token[i] == 'f'){
                    f = 1;
                }
                else if(token[i] == 'e'){
                    e = 1;
                }
                else{
                    handleError("seek: Invalid Flag");
                    return;
                }
            }
        }
        else if(strlen(target) == 0){
            strcpy(target, token);
        }
        else if(strlen(path) == 0){
            strcpy(path, token);
        }
        else{
            handleError("seek: Invalid Argument");
            return; 
        }
        token = strtok_r(NULL, " \t", &x);
    }

    if (strlen(target) == 0)
    {
        handleError("seek: missing argument");
        return;
    }

    if (d && f)
    {
        handleError("seek: Invalid Flags");
        return;
    }

    target[strcspn(target, "\n")] = '\0';
    path[strcspn(path, "\n")] = '\0';

    if (strlen(path) == 0)
    {
        strcpy(path, ".");
    }

    char resolved_path[MAX_PATH];
    strcpy(resolved_path, pathCorrect(path));

    int matches = 0;
    char single[4 * MAX_PATH] = "";

    helper(target, resolved_path, d, f, &matches, single, path);

    if (matches == 0)
    {
        handleError("seek: no matches found");
    }
    else if (e && matches == 1)
    {
        struct stat st;
        if (stat(single, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                if (chdir(single) == 0)
                {
                    printf("Changed directory to %s\n", single);
                }
                else
                {
                    handleError("seek: Missing permission to change directory");
                }
            }
            else
            {
                printfilecontents(single);
            }
        }
        else
        {
            handleError("stat");
        }
    }
}