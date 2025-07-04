#include "../include/config.h"
#include "../utils/string_utils.h" // For trimstr
#include <stdio.h>  // For FILE, fopen, fgets, fclose, fprintf, stderr
#include <stdlib.h> // For malloc, free, exit, EXIT_FAILURE
#include <string.h> // For strdup, strcmp, strtok, strstr, strcspn, snprintf

Alias *alias_list = NULL;
Function *function_list = NULL;

// Adds a new alias to the list
void add_alias(const char *name, const char *command)
{
    Alias *new_alias = (Alias *)malloc(sizeof(Alias));
    new_alias->name = strdup(name);
    new_alias->command = strdup(command);
    new_alias->next = alias_list;
    alias_list = new_alias;
}

char *find_alias(const char *name)
{
    Alias *current = alias_list;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current->command;
        }
        current = current->next;
    }
    return NULL;
}

void replace_alias(char *command)
{
    
    char *alias_command = find_alias(command);
    if (alias_command != NULL)
    {
        strcpy(command, alias_command);
    }

    char *new_cmd = (char *)malloc(MAX_COMMAND);
    if (new_cmd == NULL)
    {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    int j = 0;
    for (int i = 0; i < strlen(command); i++)
    {
        if (command[i] != '\'')
        {
            new_cmd[j++] = command[i];
        }
    }
    new_cmd[j] = '\0';

    strcpy(command, new_cmd);
    free(new_cmd);
}

void add_function(const char *name, const char *definition)
{
    Function *new_function = (Function *)malloc(sizeof(Function));
    new_function->name = strdup(name);
    new_function->definition = strdup(definition);
    new_function->next = function_list;
    function_list = new_function;
}

char *find_function(const char *name)
{
    Function *current = function_list;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current->definition;
        }
        current = current->next;
    }
    return NULL;
}

void execute_function(const char *name, const char *arg)
{
    char *definition = find_function(name);
    if (definition != NULL)
    {
        char *command;
        char *x;
        command = strtok_r(definition, "\n", &x);
        while (command != NULL)
        {
            trimstr(command);
            if (strstr(command, "{") != NULL || strstr(command, "}") != NULL)
            {
                command = strtok_r(NULL, "\n", &x);
                continue;
            }

            for (int i = 0; i < strlen(command); i++)
            {
                if (command[i] == '#')
                {
                    command[i] = '\0';
                    break;
                }
            }

            char *pos = strstr(command, "\"$1\"");
            if (pos != NULL)
            {
                char temp[MAX_COMMAND];
                strncpy(temp, command, pos - command);
                temp[pos - command] = '\0';
                snprintf(temp + (pos - command), sizeof(temp) - (pos - command), "%s%s", arg, pos + 4);
                strcpy(command, temp);
            }
            // executeCommand(NULL, command);
            executeInFG(command);
            command = strtok_r(NULL, "\n", &x);
        }
    }
    else
    {
        fprintf(stderr, "Function %s not found\n", name);
    }
}

void load_config(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Could not open configuration file");
        return;
    }

    char line[MAX_COMMAND];
    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = 0;
        if (line[0] == '\0' || line[0] == '#')
        {
            continue;
        }

        if (strncmp(line, "alias ", 6) == 0)
        {
            char *name = strtok(line + 6, "=");
            char *command = strtok(NULL, "=");
            if (name && command)
            {
                add_alias(name, command);
            }
        }

        if (strstr(line, "()") != NULL || strstr(line, "(){") != NULL)
        {
            trimstr(line);
            char *name = strtok(line, " (");
            char *real_name = (char *)malloc(strlen(name) + 1);
            strncpy(real_name, name, strlen(name));
            real_name[strlen(name)] = '\0';

            if (name)
            {
                char definition[MAX_COMMAND] = "";
                while (fgets(line, sizeof(line), file))
                {
                    strcat(definition, line);
                    if (strchr(line, '}'))
                    {
                        break;
                    }
                }
                definition[strlen(definition) - 1] = '\0';
                add_function(real_name, definition);
            }
        }
    }

    fclose(file);
}