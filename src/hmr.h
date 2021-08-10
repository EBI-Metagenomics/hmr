#ifndef HMR_H
#define HMR_H

#include "fsm.h"
#include "token.h"
#include <stdio.h>

struct hmr_aux
{
    union
    {
        struct
        {
            char *begin;
            char *pos;
            char *end;
        } prof;

        struct
        {
            unsigned idx;
            double *begin;
            double *pos;
            double *end;
        } node;
    };
};

void hmr_aux_reset(struct hmr_aux *aux);

#endif
