#include "log.h"

CommandLog *initLog()
{
    CommandLog *log = (CommandLog *)malloc(sizeof(CommandLog));
    log->lineCount = 0;
    log->prevCommand[0] = '\0';
    strcpy(log->pathtohistory, logfile);
    log->fd = open(log->pathtohistory, O_CREAT | O_RDWR | O_APPEND, 0644);
    if(log->fd == -1)
    {
        handleError("opening log file failed");
        return NULL;
    }

    char fileContent[15 * MAX_COMMAND];
    int bytes = 0;
    int totalBytes = 0;

    while((bytes = read(log->fd, fileContent + totalBytes, sizeof(fileContent) - totalBytes)) > 0)
    {
        totalBytes += bytes;
    }

    if(bytes == -1)
    {
        handleError("read");
        return NULL;
    }

    char *line = strtok(fileContent, "\n");
    while(line != NULL)
    {
        log->lineCount++;
        strncpy(log->prevCommand, line, sizeof(log->prevCommand) - 1);
        log->prevCommand[strlen(log->prevCommand) - 1] = '\0';
        line = strtok(NULL, "\n");
    }

    lseek(log->fd, 0, SEEK_END);
    return log;
}

char *check_prevCommand(const char *command){
    FILE *file = fopen(logfile, "r");
    if (file == NULL) {
        perror("Failed to open history.txt");
        return NULL;
    }
    char lastCommand[MAX_COMMAND] = {0};
    char line[MAX_COMMAND];
    while (fgets(line, sizeof(line), file) != NULL) {
        strcpy(lastCommand, line);
    }
    fclose(file);
    int len = strlen(lastCommand);
    if (len > 0 && lastCommand[len - 1] == '\n') {
        lastCommand[len - 1] = '\0';
    }

    return strdup(lastCommand);
}

void addCommand(const char *command)
{
    if(strcmp(history->prevCommand, command) == 0)
    {
        return;
    }
    char *last = check_prevCommand(command);
    if(last != NULL && strcmp(last, command) == 0)
    {
        return;
    }

    FILE *file = fopen(logfile, "r");
    if (file == NULL) {
        perror("Failed to open history.txt");
        return;
    }
    char lastCommand[MAX_COMMAND] = {0};
    char line[MAX_COMMAND];
    while (fgets(line, sizeof(line), file) != NULL) {
        strcpy(lastCommand, line);
    }
    fclose(file);
    int len = strlen(lastCommand);
    if (len > 0 && lastCommand[len - 1] == '\n') {
        lastCommand[len - 1] = '\0';
    }

    if (strcmp(lastCommand, command) == 0) {
        return;
    }

    if(history->lineCount < MAX_HISTORY)
    {
        write(history->fd, command, strlen(command));
        write(history->fd, "\n", 1);
        history->lineCount++;
    }
    else
    {
        FILE *file = fopen(history->pathtohistory, "r");
        if (!file)
        {
            handleError("fopen");
            return;
        }

        FILE *tempFile = fopen(tempfile_path, "w");
        if (!tempFile)
        {

            handleError("fopen");
            fclose(file);
            return;
        }

        char line[MAX_COMMAND];
        int count = 0;
        while (fgets(line, sizeof(line), file))
        {
            if (count > 0)
            {
                fputs(line, tempFile);
            }
            count++;
        }

        fclose(file);
        fclose(tempFile);

        remove(history->pathtohistory);
        rename(tempfile_path, history->pathtohistory);

        history->fd = open(history->pathtohistory, O_CREAT | O_RDWR | O_APPEND, 0644);
        if(history->fd == -1)
        {
            handleError("open");
            return;
        }

        write(history->fd, command, strlen(command));
        write(history->fd, "\n", 1);
    }
    strncpy(history->prevCommand, command, sizeof(history->prevCommand) - 1);
}

void removeCommand()
{

    if(history->lineCount == 0)
    {
        return;
    }

    FILE *file = fopen(history->pathtohistory, "r");
    if (!file)
    {
        handleError("fopen failed in removing command");
        return;
    }
    FILE *tempFile = fopen(tempfile_path, "w");
    if (!tempFile)
    {
        handleError("fopen failed in removing command");
        fclose(file);
        return;
    }

    char line[MAX_COMMAND];
    int count = 0;
    history->prevCommand[0] = '\0';
    while (fgets(line, sizeof(line), file))
    {
        if (count < history->lineCount - 1)
        {
            fputs(line, tempFile);
        }
        count++;
    }

    fclose(file);
    fclose(tempFile);

    int rc = remove(history->pathtohistory);
    if(rc != 0)
    {
        handleError("remove() failed in removing command");
        return;
    }
    int rc2 = rename(tempfile_path, history->pathtohistory);
    if(rc2 != 0)
    {
        handleError("rename() failed in removing command");
        return;
    }

    history->fd = open(history->pathtohistory, O_CREAT | O_RDWR | O_APPEND, 0644);
    if(history->fd == -1)
    {
        handleError("open");
        return;
    }

    FILE *file2 = fopen(history->pathtohistory, "r");
    if (!file2)
    {
        handleError("fopen failed in removing command");
        return;
    }

    char line2[MAX_COMMAND];
    int count2 = 0;
    while (fgets(line2, sizeof(line2), file2))
    {
        if(count2 == history->lineCount - 2){
            strncpy(history->prevCommand, line2, sizeof(history->prevCommand) - 1);
            history->prevCommand[strlen(history->prevCommand) - 1] = '\0';
        }
        count2++;
    }

    history->lineCount--;
}

void printLog(){
    if(history->lineCount == 0)
    {
        return;
    }
    
    FILE *file = fopen(history->pathtohistory, "r");
    if (file == NULL)
    {
        handleError("fopen failed in printing history");
        return;
    }

    char line[MAX_COMMAND];
    int count = 0;
    while (fgets(line, sizeof(line), file))
    {
        printf("%d. %s", count + 1, line);
        count++;
    }
    fclose(file);
}

void purgeLog(){
    FILE *file = fopen(history->pathtohistory, "w");
    if (!file)
    {
        handleError("fopen failed in purging history");
        return;
    }
    fclose(file);
    history->lineCount = 0;
    history->prevCommand[0] = '\0';
}

char *executeLog(int index){
    FILE *file = fopen(history->pathtohistory, "r");
    if (!file)
    {
        handleError("fopen failed in executeLog");
        return NULL;
    }

    char line[MAX_COMMAND];
    int count = 0;
    while (fgets(line, sizeof(line), file))
    {
        if(count == history->lineCount - index){
            fclose(file);
            return strdup(line);
        }
        count++;
    }

    fclose(file);
    return NULL;
}

void freeLog(){
    close(history->fd);
    free(history);
}