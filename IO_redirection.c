#include "IO_redirection.h"

char* handleIO(char *command_got){

    char *command = (char *)malloc(MAX_COMMAND);
    strcpy(command, command_got);
    char *modified_command = (char *)malloc(MAX_COMMAND);


    // printf("command: %s\n", command);
    char *tempPt = NULL;

    if (strstr(command, "<<") != NULL)
    {
        handleError("Unsupported I/O redirection");
        return NULL;
    }
    tempPt = strstr(command, "<");
    if (tempPt != NULL)
    {
        char *temp = (char *)malloc((strlen(command) + 1) * sizeof(char));
        if (temp == NULL)
        {
            handleError("Error allocating memory");
            return NULL;
        }
        strcpy(temp, command);

        char *x;
        char *part1 = strtok_r(temp, "<", &x);
        char *part2 = tempPt + 1;

        // need to take input file from part2
        if (part2 == NULL)
        {
            handleError("Invalid I/O redirection");
            free(temp);
            return NULL;
        }
        trimstr(part1);
        trimstr(part2);

        input_file = strtok(part2, " \t\n");
        if (input_file == NULL)
        {
            handleError("Invalid I/O redirection");
            free(temp);
            return NULL;
        }
        trimstr(input_file);
        input_file = pathCorrect(input_file);
        printf("input file: %s\n", input_file);
        if (freopen(input_file, "r", stdin) == NULL)
        {
            handleError("Input File Not Found");
            free(temp);
            return NULL;
        }
        free(temp);
    }

    tempPt = strstr(command, ">");
    if (tempPt != NULL)
    {
        // two possibilities > or >>
        char *tempPt2 = strstr(command, ">>");
        if (tempPt2 != NULL)
        {
            char *temp = (char *)malloc((strlen(command) + 1) * sizeof(char));
            if (temp == NULL)
            {
                handleError("Error allocating memory");
                return NULL;
            }
            strcpy(temp, command);

            char *x;
            char *part1 = strtok_r(temp, ">>", &x);

            char *part2 = tempPt2 + 2;
            if (part2 == NULL)
            {
                handleError("Invalid I/O redirection");
                free(temp);
                return NULL;
            }
            trimstr(part2);

            output_file = strtok(part2, " \t\n");

            if (output_file == NULL)
            {
                handleError("Invalid I/O redirection");
                free(temp);
                return NULL;
            }
            trimstr(output_file);
            output_file = pathCorrect(output_file);

            if (freopen(output_file, "a", stdout) == NULL)
            {
                handleError("Output File Not Found");
                free(temp);
                return NULL;
            }
            free(temp);
        }
        else
        {
            char *temp = (char *)malloc((strlen(command) + 1) * sizeof(char));
            if (temp == NULL)
            {
                handleError("Error allocating memory");
                return NULL;
            }
            strcpy(temp, command);

            char *x;
            char *part1 = strtok_r(temp, ">", &x);
            char *part2 = tempPt + 1;
            if (part2 == NULL)
            {
                handleError("Invalid I/O redirection");
                free(temp);
                return NULL;
            }
            trimstr(part2);
        
            output_file = strtok(part2, " \t\n");

            if (output_file == NULL)
            {
                handleError("Invalid I/O redirection");
                free(temp);
                return NULL;
            }
            trimstr(output_file);
            output_file = pathCorrect(output_file);

            if (freopen(output_file, "w", stdout) == NULL)
            {
                handleError("Output File Not Found");
                free(temp);
                return NULL;
            }
            free(temp);
        }
        if(!tempPt2)
            free(tempPt2);
    }
    trimstr(command);
    if(!tempPt)
        free(tempPt);

    modified_command = strtok(command, "><\t\n");
    return modified_command;
}

void resetIO(){

    if(freopen("/dev/tty", "r", stdin) == NULL){
        handleError("Error restoring stdin");
    }
    if(freopen("/dev/tty", "w", stdout) == NULL){
        handleError("Error restoring stdout");
    }
    if(freopen("/dev/tty", "w", stderr) == NULL){
        handleError("Error restoring stderr");
    }
    return;
}