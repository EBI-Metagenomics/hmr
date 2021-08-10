#ifndef HMR_HMR_H
#define HMR_HMR_H

#include "hmr/export.h"
#include "hmr/node.h"
#include "hmr/prof.h"
#include "hmr/rc.h"
#include <stdio.h>

struct hmr;
struct hmr_prof;

HMR_API struct hmr *hmr_new(void);
HMR_API enum hmr_rc hmr_open(struct hmr *hmr, FILE *restrict fd);
HMR_API enum hmr_rc hmr_read(struct hmr *hmr, struct hmr_prof *prof);
HMR_API void hmr_close(struct hmr *hmr);
HMR_API void hmr_del(struct hmr const *hmr);

#endif
