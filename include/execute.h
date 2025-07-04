#ifndef EXECUTE_H
#define EXECUTE_H

#include "../utils/global.h" // Already changed
#include "proclore.h"
#include "log.h"
#include "seek.h"
#include "hop.h"
#include "reveal.h"
// #include "promt.h" // Removed, promt.c and promt.h deleted
#include "help.h"
#include "bg.h"
#include "syscmd.h"
#include "activities.h"
#include "iman.h"
#include "signal.h"
#include "fgbg.h"
#include "neonate.h"
#include "fg.h"
// If display_prompt was needed here, it would be #include "../utils/prompt_utils.h"

// void executeInFG(CommandLog *log, char *command);
// void formatter_executer(char *command_input);
void executeCommand(char *command_got);

#endif