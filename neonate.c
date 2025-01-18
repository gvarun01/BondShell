#include "neonate.h"

int get_most_recent_pid()
{
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL)
    {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    int most_recent_pid = -1;
    time_t most_recent_time = 0;

    while ((entry = readdir(proc_dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            int pid = atoi(entry->d_name);
            if (pid > 0)
            {
                char path[256];
                snprintf(path, sizeof(path), "/proc/%d", pid);
                struct stat statbuf;
                if (stat(path, &statbuf) == 0)
                {
                    if (statbuf.st_ctime > most_recent_time)
                    {
                        most_recent_time = statbuf.st_ctime;
                        most_recent_pid = pid;
                    }
                }
            }
        }
    }

    closedir(proc_dir);
    return most_recent_pid;
}

int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

void neonate(char *subcom_input)
{
    char *subcom = (char *)malloc(MAX_COMMAND * sizeof(char));
    strcpy(subcom, subcom_input);

    if (strstr(subcom, "-n") == NULL)
    {
        handleError("neonate: Invalid command");
        return;
    }
    trimstr(subcom);
    char *command = strtok(subcom, " \t\n");
    if (strcmp(command, "-n") != 0)
    {
        handleError("neonate: Invalid command");
        return;
    }
    command = strtok(NULL, " \t\n");
    if (command == NULL)
    {
        handleError("neonate: Invalid command");
        return;
    }
    char *additional = strtok(NULL, " \t\n");
    if (additional != NULL)
    {
        handleError("neonate: Additional Arguments not allowed");
        return;
    }
    int time = atoi(command);
    if (time < 0)
    {
        handleError("neonate: Invalid time");
        return;
    }

    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    sigset_t new_set, old_set;
    sigemptyset(&new_set);
    sigaddset(&new_set, SIGINT);
    sigaddset(&new_set, SIGTSTP);
    if (sigprocmask(SIG_BLOCK, &new_set, &old_set) < 0)
    {
        handleError("neonate: sigprocmask failed");
        exit(EXIT_FAILURE);
    }

    printf(COLOR_MAGENTA "Neonate mode activated. Press " COLOR_RESET COLOR_YELLOW "'x'" COLOR_MAGENTA " to exit.\n" COLOR_RESET);

    while (1)
    {
        int pid = get_most_recent_pid();
        if (pid == -1)
        {
            handleError("neonate: get_most_recent_pid failed");
            break;
        }
        printf("%d\n", pid);
        fflush(stdout);

        if (time == 0)
        {
            if (kbhit())
            {
                char c = getchar();
                if (c == 'x')
                {
                    break;
                }
            }
        }
        else
        {
            bool flag = false;
            for (long long int i = 0; i < time * 1000; i++)
            {
                if (kbhit())
                {
                    char c = getchar();
                    if (c == 'x')
                    {
                        flag = true;
                        break;
                    }
                }
                usleep(1000);
            }
            if (flag == true)
            {
                // x is pressed
                break;
            }
        }

        // sleep(time);
        // if(kbhit()){
        //     char c = getchar();
        //     if(c == 'x'){
        //         break;
        //     }
        // }
    }

    if (sigprocmask(SIG_SETMASK, &old_set, NULL) < 0)
    {
        // perror("sigprocmask");
        handleError("neonate: sigprocmask failed");
        exit(EXIT_FAILURE);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    printf(COLOR_MAGENTA "Neonate mode deactivated.\n" COLOR_RESET);
    return;
}