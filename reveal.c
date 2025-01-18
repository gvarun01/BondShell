#include "reveal.h"

void printName(char *name, mode_t mode)
{
    if (S_ISDIR(mode))
    {
        printf(COLOR_BLUE "%s" COLOR_RESET, name);
    }
    else if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
    {
        printf(COLOR_GREEN "%s" COLOR_RESET, name);
    }
    else
    {
        printf("%s", name);
    }
}

int compare(const void *a, const void *b)
{
    const char *stra = *(const char **)a;
    const char *strb = *(const char **)b;

    if (strcmp(stra, ".") == 0 && strcmp(strb, "..") == 0)
    {
        return -1;
    }
    if (strcmp(stra, "..") == 0 && strcmp(strb, ".") == 0)
    {
        return 1;
    }

    while (*stra == '.' || *stra == '/')
    {
        stra++;
    }
    while (*strb == '.' || *strb == '/')
    {
        strb++;
    }

    while (*stra && *strb)
    {
        char charA = tolower(*stra);
        char charB = tolower(*strb);

        if (charA != charB)
        {
            return charA - charB;
        }
        if (*stra != *strb)
        {
            return *stra - *strb;
        }
        stra++;
        strb++;
    }

    return *stra - *strb;

    return strcmp(stra, strb);
}

void print_permissions(mode_t mode)
{
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

void printFiles(char *path, char *name, bool hidden, bool long_format)
{
    char fullPath[4 * MAX_PATH];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, name);

    struct stat fileStat;
    if (lstat(fullPath, &fileStat) < 0)
    {
        handleError("lstat failed in reveal");
        return;
    }

    if (!hidden && name[0] == '.')
    {
        return;
    }

    if (long_format)
    {
        struct passwd *pw = getpwuid(fileStat.st_uid);
        struct group *gr = getgrgid(fileStat.st_gid);
        char time[100];
        strftime(time, sizeof(time), "%b %d %H:%M", localtime(&fileStat.st_mtime));

        print_permissions(fileStat.st_mode);
        printf(" %2ld", fileStat.st_nlink);
        printf(" %s", pw->pw_name);
        printf(" %s", gr->gr_name);
        printf(" %8ld", fileStat.st_size);
        printf(" %s ", time);
        printName(name, fileStat.st_mode);
        printf("\n" COLOR_RESET);
    }
    else
    {
        printName(name, fileStat.st_mode);
        printf("\n");
    }
}

void reveal(char *str)
{
    char flag[MAX_NAME] = "";
    char path[MAX_PATH + 1] = "";

    bool hidden = false;
    bool long_format = false;

    char *x;
    char *token = strtok_r(str, " \t", &x);
    int cnt = 0;
    while (token != NULL)
    {
        if(strncmp(token, "--", 2) == 0){
            handleError("reveal: Invalid Flag");
            return;
        }

        else if(strncmp(token, "-", 1) == 0 && cnt == 0){
            cnt ++;
            for(int i = 1; i < strlen(token); i++){
                if(token[i] == '-'){
                    continue;
                }
                else if(token[i] == 'a'){
                    hidden = true;
                }
                else if(token[i] == 'l'){
                    long_format = true;
                }
                else{
                    handleError("reveal: Invalid Flag");
                    return;
                }
            }
        }
        else if(strlen(path) == 0){
            strcpy(path, token);
        }
        else{
            handleError("reveal: Invalid Argument");
            return; 
        }
        token = strtok_r(NULL, " \t", &x);
    }

    if(strlen(path) == 0){
        strcpy(path, ".");
    }

    char newPath[2 * MAX_PATH];
    strcpy(newPath, pathCorrect(path));

    DIR *dir = opendir(newPath);
    if (dir == NULL)
    {
        handleError("opendir");
        return;
    }

    struct dirent *entry;
    char *entries[1024];
    int total_count = 0;
    int total = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!hidden && entry->d_name[0] == '.')
        {
            continue;
        }
        entries[total_count++] = strdup(entry->d_name);

        struct stat file_stat;
        char full_path[4 * MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", newPath, entry->d_name);
        if (stat(full_path, &file_stat) == 0)
        {
            total += file_stat.st_blocks;
        }
    }

    closedir(dir);

    qsort(entries, total_count, sizeof(char *), compare);

    if (long_format)
    {
        printf("total %d\n", total / 2);
    }

    for (int i = 0; i < total_count; i++)
    {
        printFiles(newPath, entries[i], hidden, long_format);
    }

    for (int i = 0; i < total_count; i++)
    {
        free(entries[i]);
    }
}