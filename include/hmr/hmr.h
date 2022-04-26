#ifndef HMR_HMR_H
#define HMR_HMR_H

#include "hmr/aux.h"
#include "hmr/error.h"
#include "hmr/export.h"
#include "hmr/node.h"
#include "hmr/prof.h"
#include "hmr/rc.h"
#include "hmr/state.h"
#include "hmr/tok.h"
#include "hmr/trans.h"
#include <stdio.h>

struct hmr
{
    FILE *restrict fp;
    enum hmr_state state;
    struct hmr_tok tok;
    struct hmr_aux aux;
    char error[HMR_ERROR_SIZE];
};

#define HMR_DECLARE(name, fp)                                                  \
    struct hmr name;                                                           \
    hmr_init(&name, fp)

HMR_API void hmr_init(struct hmr *hmr, FILE *restrict fp);
HMR_API enum hmr_rc hmr_next_prof(struct hmr *hmr, struct hmr_prof *prof);
HMR_API enum hmr_rc hmr_next_node(struct hmr *hmr, struct hmr_prof *prof);
HMR_API void hmr_clear_error(struct hmr *hmr);

#endif
