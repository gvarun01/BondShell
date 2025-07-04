#include "error_handler.h"
#include <stdio.h> // For fprintf, stderr

// Prints an error message to stderr, formatted with a specific color
void handleError(const char *message)
{
    // Using COLOR_ERROR which is defined as COLOR_YELLOW in global.h
    // It might be better to define a specific ERROR_COLOR or pass color as parameter if needed.
    fprintf(stderr, COLOR_ERROR "Error: %s\n" COLOR_RESET, message);
}
