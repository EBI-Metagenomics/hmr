#ifndef HMR_PROF_H
#define HMR_PROF_H

#include "hmr/export.h"
#include "hmr/node.h"
#include "hmr/rc.h"
#include <stdio.h>

struct hmr_prof
{
    char header[HMR_HEADER_MAX];
    struct
    {
        char name[HMR_NAME_MAX];
        char acc[HMR_ACC_MAX];
        char desc[HMR_DESC_MAX];
        char leng[HMR_LENG_MAX];
        char alph[HMR_ALPH_MAX];
    } meta;
    unsigned symbols_size;
    char symbols[HMR_SYMBOLS_MAX];

    struct hmr_node node;
};

#define HMR_PROF_DECLARE(name)                                                 \
    struct hmr_prof name;                                                      \
    hmr_prof_init(&name)

HMR_API void hmr_prof_dump(struct hmr_prof const *prof, FILE *fd);
HMR_API void hmr_prof_init(struct hmr_prof *prof);
HMR_API enum hmr_rc hmr_prof_read(struct hmr_prof *prof);

#endif
