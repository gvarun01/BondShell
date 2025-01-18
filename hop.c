#include "hop.h"

void hop(char *str_input){
    if(str_input == NULL){
        // handleError("No path provided");
        if(chdir(home) == -1){
            handleError("chdir failed");
            return;
        }
        strcpy(lastDir, home);
        printf("%s\n", home);
        return;
    }

    char *str = (char *)malloc(MAX_PATH);
    strcpy(str, str_input);
    char current[MAX_PATH];
    if (getcwd(current, sizeof(current)) == NULL) {
        handleError("getcwd failed in hop");
        return;
    }

    if (str == NULL || strlen(str) == 0)
    {
        if (chdir(home) == -1) {
            handleError("chdir failed");
            return;
        }
        getcwd(current, sizeof(current));
        printf("%s\n", current);
        return;
    }

    char *targets[MAX_HOP];
    
    int count = 0;

    char *token = strtok(str, " ");
    while (token != NULL)
    {
        targets[count] = (char *)malloc(MAX_PATH * sizeof(char));
        strcpy(targets[count], token);
        count++;
        token = strtok(NULL, " ");
    }
    free(str);


    int i = 0;
    for(; i < count; i++){
        char path[MAX_PATH];
        strcpy(path, targets[i]);

        char newPath[2*MAX_PATH];
        strcpy(newPath, pathCorrect(path));

        if(strcmp(path, "-") == 0){
            if (strlen(lastDir) == 0) {
                handleError("No previous directory");
                continue;
            }
            strcpy(newPath, lastDir);
        }

        if (chdir(newPath) == -1) {
            char error[MAX_PATH];
            strcpy(error, "chdir failed for ");
            strcat(error, path);
            handleError(error);
            continue;
        }

        strcpy(lastDir, current);
        if (getcwd(current, sizeof(current)) == NULL) {
            handleError("getcwd failed in hop");
            return;
        }
        printf("%s\n", current);
        fflush(stdout);
    }
    for(int j = 0; j < i; j++){
        free(targets[j]);
    }
    fflush(stdout);
    return;
}