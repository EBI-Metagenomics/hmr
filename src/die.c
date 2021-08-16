#include "die.h"
#include <stdio.h>
#include <stdlib.h>

void __die(char const *file, int line)
{
    fprintf(stderr, "DIE: %s:%d\n", file, line);
    fflush(stderr);
    exit(EXIT_FAILURE);
}
