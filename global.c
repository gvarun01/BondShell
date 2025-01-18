#include "global.h"

char username[MAX_NAME];
char sysname[MAX_NAME];
char wrkdirectory[MAX_PATH];
char home[MAX_PATH];

int executionTime = 0;
char executedCommand[MAX_COMMAND] = "";
char promtExec[MAX_COMMAND] = "";
char lastDir[MAX_PATH] = "";
char logfile[MAX_PATH] = "";
char tempfile_path[MAX_PATH] = "";

void handleError(const char *message)
{
    fprintf(stderr, COLOR_ERROR "Error: %s\n" COLOR_RESET, message);
}

void initialise()
{
    if (getlogin_r(username, MAX_NAME) != 0)
    {
        printf("Error getting username.\n");
        exit(1);
    }

    if (gethostname(sysname, MAX_NAME) != 0)
    {
        printf("Error getting system name.\n");
        exit(1);
    }

    if (getcwd(wrkdirectory, MAX_PATH) == NULL)
    {
        printf("Error getting path.\n");
        exit(1);
    }

    strcpy(home, wrkdirectory);

    strcat(logfile, home);
    strcat(tempfile_path, home);
    strcat(logfile, "/history.txt");
    strcat(tempfile_path, "/temp.txt");
}

void trimstr(char *str)
{
    if (str == NULL)
        return;

    char *start = str;
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n'))
        ++start;

    char *end = start + strlen(start) - 1;
    while (end >= start && (*end == ' ' || *end == '\t' || *end == '\n'))
        --end;

    int length = end - start + 1;
    memmove(str, start, length);
    str[length] = '\0';
}

char *pathCorrect(char *path)
{
    char *newPath = (char *)malloc(2 * MAX_PATH);
    if (strcmp(path, "~") == 0)
    {
        strcpy(newPath, home);
    }
    else if (strncmp(path, "~", 1) == 0)
    {
        char path2[MAX_PATH];
        strcpy(path2, path + 1);
        strcpy(newPath, home);
        strcat(newPath, path2);
    }
    else if (strcmp(path, ".") == 0)
    {
        if (getcwd(newPath, MAX_PATH) == NULL)
        {
            handleError("getcwd");
            return NULL;
        }
    }

    else if (strcmp(path, "..") == 0)
    {
        if (getcwd(newPath, MAX_PATH) == NULL)
        {
            handleError("getcwd");
            return NULL;
        }
        char *x = strrchr(newPath, '/');
        if (x == newPath)
        {
            newPath[1] = '\0';
        }
        else
        {
            *x = '\0';
        }
    }
    else
    {
        if (path[0] != '/')
        {
            if (getcwd(newPath, MAX_PATH) == NULL)
            {
                handleError("getcwd");
                return NULL;
            }
            strcat(newPath, "/");
            strcat(newPath, path);
        }
        else
            strcpy(newPath, path);
    }
    return newPath;
}

char *relativePath(char *path)
{
    char *newPath = (char *)malloc(2 * MAX_PATH);
    if (strncmp(path, home, strlen(home)) == 0)
    {
        strcpy(newPath, "~");
        strcat(newPath, path + strlen(home));
    }
    else
    {
        strcpy(newPath, path);
    }
    return newPath;
}

char *getcmd(char *cmd)
{
    char *newCmd = (char *)malloc(MAX_COMMAND);
    strcpy(newCmd, cmd);
    char *x;
    char *cmd1 = strtok_r(newCmd, " \t\n", &x);
    if (cmd1 == NULL)
    {
        return NULL;
    }
    return cmd1;
}

void intro()
{
    // printf(" ____   ___  _   _ ____     ____  _   _ _____ _     _\n");
    // printf("| __ ) / _ \| \ | |  _ \   / ___|| | | | ____| |   | |\n");
    // printf("|  _ \| | | |  \| | | | |  \___ \| |_| |  _| | |   | |\n");
    // printf("| |_) | |_| | |\  | |_| |   ___) |  _  | |___| |___| |___\n");
    // printf("|____/ \___/|_| \_|____/   |____/|_| |_|_____|_____|_____|\n");
    // printf("\t\t\t ____   ___  _   _ ____     ____  _   _ _____ _     _\n");
    // printf("\t\t\t| __ ) / _ \\| \\ | |  _ \\   / ___|| | | | ____| |   | |\n");
    // printf("\t\t\t|  _ \\| | | |  \\| | | | |  \\___ \\| |_| |  _| | |   | |\n");
    // printf("\t\t\t| |_) | |_| | |\\  | |_| |   ___) |  _  | |___| |___| |___\n");
    // printf("\t\t\t|____/ \\___/|_| \\_|____/   |____/|_| |_|_____|_____|_____|\n");
    // printf(COLOR_BRIGHT_RED "\t\t\t\t\tLoading");
    // for (int i = 0; i < 10; i++) {
    //     printf(".");
    //     fflush(stdout); // Ensure the dot is printed immediately
    //     usleep(500000); // Sleep for 500 milliseconds
    // }
    // system("clear");
    // printf(COLOR_RESET);

    printf(STYLE_BOLD COLOR_MAGENTA "\033[?25l\n");
    printf("\t\t\t            ____                  _   ____  _          _ _\n");
    usleep(500000);
    printf("\t\t\t           | __ )  ___  _ __   __| | / ___|| |__   ___| | |\n");
    usleep(500000);
    printf("\t\t\t           |  _ \\ / _ \\| '_ \\ / _` | \\___ \\| '_ \\ / _ \\ | |\n");
    usleep(500000);
    printf("\t\t\t           | |_) | (_) | | | | (_| |  ___) | | | |  __/ | |\n");
    usleep(500000);
    printf("\t\t\t           |____/ \\___/|_| |_|\\__,_| |____/|_| |_|\\___|_|_|\n");
    usleep(500000);
    printf("\n");
    printf("\033[?25h" COLOR_RESET);

    printf(COLOR_BLUE STYLE_BOLD "\nWelcome to BOND Shell 🛡️ 👾,\n");
    printf("\ta simple Interactive shell written in C.\n" COLOR_RESET);
    printf("Type ");
    printf(COLOR_YELLOW "help" COLOR_RESET);
    printf(" to list all the available commands.\n");
    printf("Type ");
    printf(COLOR_YELLOW "exit" COLOR_RESET);
    printf(" to exit the shell.\n");
    printf("Crafted with " COLOR_RED STYLE_BOLD "❤️" COLOR_RESET " by Varun Gupta.\n\n");
}

void swap_c(char *a, char *b)
{
    char temp[MAX_PATH];
    strcpy(temp, a);
    strcpy(a, b);
    strcpy(b, temp);
}

void exitCheck(char *command)
{
    char *cpy = (char *)malloc(MAX_COMMAND);
    strcpy(cpy, command);
    trimstr(cpy);
    if (strcmp(cpy, "exit") == 0)
    {
        printf(COLOR_YELLOW "Exiting shell.....\n" COLOR_RESET);
        killAllProcesses();
        exit(EXIT_SUCCESS);
    }
}