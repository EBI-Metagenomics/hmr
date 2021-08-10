#ifndef HMR_PROF_H
#define HMR_PROF_H

#include "hmr/export.h"
#include "hmr/node.h"
#include "hmr/rc.h"

struct hmr_prof
{
    char header[HMR_HEADER_MAX];
    struct
    {
        char NAME[HMR_NAME_MAX];
        char ACC[HMR_ACC_MAX];
        char DESC[HMR_DESC_MAX];
        char LENG[HMR_LENG_MAX];
        char ALPH[HMR_ALPH_MAX];
    } meta;
    unsigned symbols_size;
    char symbols[HMR_SYMBOLS_MAX];

    struct hmr_node node;

    struct
    {
        char *begin;
        char *pos;
        char *end;
    } __;
};

HMR_API enum hmr_rc hmr_prof_read(struct hmr_prof *prof);

#endif
