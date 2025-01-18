#include "promt.h"

// void initialize_promt();

void promt(char *promtExec){
    char cwd[MAX_PATH];
    if(getcwd(cwd, sizeof(cwd)) == NULL){
        handleError("getcwd failed printing promt");
        return;
    }

    char *line;
    line = relativePath(cwd);

    printf( "<" STYLE_BOLD COLOR_RED "%s" COLOR_RESET  STYLE_BOLD "@" COLOR_GREEN "%s" COLOR_RESET STYLE_BOLD ":" COLOR_MAGENTA "%s" COLOR_RESET, username, sysname, line);
    if(strlen(promtExec) > 0){
        printf(COLOR_YELLOW " %s " COLOR_RESET, promtExec);
    }
    printf("> ");
}