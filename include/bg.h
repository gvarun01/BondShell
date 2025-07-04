#ifndef BG_H
#define BG_H

#include "../utils/global.h"
#include "syscmd.h"
#include "IO_redirection.h"

void bg(char *subcom_input);
void checkbg();
void delete_bg(int pid);

#endif