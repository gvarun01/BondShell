#ifndef SHELL_UTILS_H
#define SHELL_UTILS_H

#include "global.h"

void exitCheck(char *command);
void swap_char_arrays(char *arr1, char *arr2); // Renamed from swap_c for clarity
// Consider if killAllProcesses from signal.c/h should be here if it's a general shell utility for exit

#endif
