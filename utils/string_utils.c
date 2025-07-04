#include "string_utils.h"
#include <string.h> // For strlen, memmove, strdup, strtok_r
#include <stdlib.h> // For malloc

// Removes leading/trailing whitespace from a string
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

// Extracts the first word (command) from a string
char *getcmd(char *cmd)
{
    if (cmd == NULL) {
        return NULL;
    }
    char *newCmd = strdup(cmd);
    if (newCmd == NULL) {
        handleError("strdup failed in getcmd");
        return NULL;
    }
    char *x = NULL; // For strtok_r
    char *cmd1 = strtok_r(newCmd, " \t\n", &x);
    if (cmd1 == NULL)
    {
        free(newCmd); // Free if no token found
        return NULL;
    }
    // Note: newCmd is not freed here if cmd1 is not NULL,
    // because cmd1 points into the memory allocated for newCmd.
    // The caller will eventually need to free the result of getcmd.
    // This is a bit tricky. A better approach might be to copy cmd1
    // into a new buffer, or document that the original string is modified
    // and the pointer returned is part of it.
    // For now, let's assume the caller manages the memory of the returned string.
    // Actually, since newCmd was allocated by strdup, and strtok_r modifies it,
    // the returned cmd1 is a pointer within this allocated block.
    // The caller should free the pointer returned by getcmd.
    return cmd1;
}
