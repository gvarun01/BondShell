#ifndef CONFIG_H
#define CONFIG_H

#include "../utils/global.h"
#include "execute.h"
#include "fg.h"

typedef struct Alias {
    char *name;
    char *command;
    struct Alias *next;
} Alias;

typedef struct Function {
    char *name;
    char *definition;
    struct Function *next;
} Function;

void add_alias(const char *name, const char *command);
char* find_alias(const char *name);
void replace_alias(char *command);
void load_config(const char *filename);

void add_function(const char *name, const char *definition);
char* find_function(const char *name);
void execute_function(const char *name, const char *arg);

#endif // CONFIG_H