#ifndef SIGNAL_H
#define SIGNAL_H

#include "global.h"
#include "promt.h"

void ping(char *subcom);
void handleCtrlC(int sig);
void killAllProcesses();
void handleCtrlZ(int sig);
#endif