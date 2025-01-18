#include "global.h"
#include "execute.h"
#include "config.h"
#include "parser.h"
#include "animation.h"

// global variables initialization
int bgcount = 0;
struct bgprocess *bgs = NULL;
int fgPid = -1;
char *fgCommand = NULL;
struct CommandLog *history = NULL;
char input[MAX_COMMAND] = "";
char *input_file = NULL;
char *output_file = NULL;
int stdout_copy_fd = -1;

int main()
{
     signal(SIGINT, handleCtrlC);

    // struct sigaction new_action, old_action;

    // new_action.sa_handler = handleCtrlZ;
    // sigemptyset(&new_action.sa_mask);
    // new_action.sa_flags = 0;

    // sigaction(SIGTSTP, NULL, &old_action);
    // if (old_action.sa_handler != SIG_IGN) {
    //     sigaction(SIGTSTP, &new_action, NULL);
    // }
    signal(SIGTSTP, handleCtrlZ);


    printf(COLOR_MAGENTA "\033[?25l"); // hide the cursor
    animation_open_close(0);

    printf("\033[?25h" COLOR_RESET); // show the cursor

    // memory allocation for global variables
    bgs = (struct bgprocess *)malloc(sizeof(struct bgprocess) * MAX_BG_PROCESSES);
    fgCommand = (char *)malloc(sizeof(char) * MAX_COMMAND);
    input_file = (char *)malloc(sizeof(char) * MAX_PATH);
    output_file = (char *)malloc(sizeof(char) * MAX_PATH);
    stdout_copy_fd = dup(fileno(stdout));

    if (bgs == NULL || fgCommand == NULL || input_file == NULL || output_file == NULL)
    {
        handleError("Memory allocation failed");
        exit(1);
    }

    if (stdout_copy_fd == -1)
    {
        handleError("Failed to copy stdout");
        exit(1);
    }

    initialise();
    history = initLog();
    intro();
    load_config(".myshrc");

   
    while (1)
    {
        setbuf(stdout, NULL);
        fgPid = -1;
        checkbg();
        promt(promtExec);
        strcpy(promtExec, "");
        fflush(stdout);

        if (fgets(input, MAX_COMMAND, stdin) == NULL)
        {
            fflush(stdout);
            killAllProcesses();
            printf(COLOR_YELLOW "\nLogging out...\n" COLOR_RESET);
            // break;
            exit(EXIT_SUCCESS);
        }
        if (strcmp(input, "\n") == 0)
        {
            continue;
        }
        input[strlen(input) - 1] = '\0';

        trimstr(input);
        replace_alias(input);

        exitCheck(input);

        char *comamnd_copy = (char *)malloc(MAX_COMMAND);
        strcpy(comamnd_copy, input);

        char *cmd_name = strtok(comamnd_copy, " ");
        char *cmd_arg = strtok(NULL, " ");
        if (find_function(cmd_name) != NULL)
        {
            execute_function(cmd_name, cmd_arg);
        }
        else
        {
            parser(input);
        }
        fflush(stdout);
        free(comamnd_copy);
    }

    free(bgs);
    free(fgCommand);
    free(input_file);
    free(output_file);
    return 0;
}