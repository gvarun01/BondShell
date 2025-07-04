#ifndef SIGNAL_H
#define SIGNAL_H

#include "../utils/global.h"
// #include "promt.h" // Removed, promt.c and promt.h deleted
// If display_prompt was needed here, it would be #include "../utils/prompt_utils.h"


void ping(char *subcom);
void handleCtrlC(int sig);
void killAllProcesses();
void handleCtrlZ(int sig);
#endif