#ifndef LOG_H
#define LOG_H

#include "../utils/global.h"

CommandLog *initLog();

void addCommand(const char *command);

void removeCommand();

void printLog();

void purgeLog();

char *executeLog(int index);

void freeLog();

#endif