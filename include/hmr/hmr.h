#ifndef HMR_HMR_H
#define HMR_HMR_H

#include "hmr/export.h"
#include "hmr/rc.h"
#include <stdio.h>

HMR_API struct hmr *hmr_open(FILE *restrict fd);
HMR_API void hmr_close(struct hmr *hmr);

#endif
