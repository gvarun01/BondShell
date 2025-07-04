#ifndef REVEAL_H
#define REVEAL_H

#include "../utils/global.h"

void swap(char *a, char *b);
int compare(const void *a, const void *b);
void printFiles(char *path, char *name, bool hidden , bool long_format);
void reveal(char *str);

#endif