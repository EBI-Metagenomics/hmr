#ifndef HMR_HMR_H
#define HMR_HMR_H

#include "hmr/aux.h"
#include "hmr/export.h"
#include "hmr/fsm.h"
#include "hmr/prof.h"
#include "hmr/token.h"
#include <stdio.h>

struct hmr
{
    FILE *restrict fd;
    enum hmr_fsm_state state;
    struct hmr_token tok;
    struct hmr_aux aux;
};

#define HMR_DECLARE(name)                                                      \
    struct hmr name;                                                           \
    hmr_init(&name)

HMR_API void hmr_init(struct hmr *hmr);
HMR_API enum hmr_rc hmr_open(struct hmr *hmr, FILE *restrict fd);
HMR_API enum hmr_rc hmr_next_prof(struct hmr *hmr, struct hmr_prof *prof);
HMR_API enum hmr_rc hmr_next_node(struct hmr *hmr, struct hmr_prof *prof);
HMR_API void hmr_close(struct hmr *hmr);

#endif
