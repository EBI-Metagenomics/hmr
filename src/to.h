#ifndef TO_H
#define TO_H

#include "hmr/rc.h"
#include <stdbool.h>

bool to_lprob(char const *str, double *val);
bool to_uint(char const *str, unsigned *val);

#endif
