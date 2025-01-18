#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "signal.h"



#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_BRIGHT_RED "\x1b[91m"
#define COLOR_ERROR COLOR_YELLOW
#define STYLE_BOLD "\x1b[1m"


#define MAX_COMMAND 4096
#define MAX_PATH 4096
#define MAX_NAME 256
#define MAX_HISTORY 15
#define MAX_HOP 100
#define MAX_BG_PROCESSES 1000
#define MAX_PIPES 100

struct bgprocess{
    pid_t id;
    char comm[MAX_COMMAND];
};

typedef struct CommandLog{
    int lineCount;
    char prevCommand[MAX_COMMAND];
    int fd;
    char pathtohistory[2*MAX_PATH];
} CommandLog;


extern char username[MAX_NAME];
extern char sysname[MAX_NAME];
extern char wrkdirectory[MAX_PATH];
extern char home[MAX_PATH];
extern int executionTime;
extern char executedCommand[MAX_COMMAND];
extern char promtExec[MAX_COMMAND];
extern char lastDir[MAX_PATH];
extern char logfile[MAX_PATH];
extern char tempfile_path[MAX_PATH];
extern int bgcount;
extern struct bgprocess *bgs;
extern int fgPid;
extern char *fgCommand;
extern struct CommandLog *history;
extern char input[MAX_COMMAND];
extern char *input_file;
extern char *output_file;
extern int stdout_copy_fd;


void initialise();
void trimstr(char *str);
void handleError(const char *message);
char *pathCorrect(char *path);
char *getcmd(char *cmd);
char *relativePath(char *path);
void intro();
void swap_c(char *a, char *b);
void exitCheck(char *command);

#endif