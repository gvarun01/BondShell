#ifndef EXECUTE_H
#define EXECUTE_H

#include "global.h"
#include "proclore.h"
#include "log.h"
#include "seek.h"
#include "hop.h"
#include "reveal.h"
#include "promt.h"
#include "help.h"
#include "bg.h"
#include "syscmd.h"
#include "activities.h"
#include "iman.h"
#include "signal.h"
#include "fgbg.h"
#include "neonate.h"
#include "fg.h"

// void executeInFG(CommandLog *log, char *command);
// void formatter_executer(char *command_input);
void executeCommand(char *command_got);

#endif