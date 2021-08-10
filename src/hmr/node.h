#ifndef HMR_NODE_H
#define HMR_NODE_H

#include "hmr/export.h"
#include <stdio.h>

#define HMR_HEADER_MAX 64
#define HMR_NAME_MAX 64
#define HMR_ACC_MAX 64
#define HMR_DESC_MAX 128
#define HMR_LENG_MAX 8
#define HMR_ALPH_MAX 12
#define HMR_SYMBOLS_MAX 32
#define HMR_TRANS_SIZE 7
#define HMR_MATCH_EXCESS_SIZE 5

struct hmr_node
{
    unsigned symbols_size;
    unsigned idx;
    union
    {
        double compo[HMR_SYMBOLS_MAX];
        double match[HMR_SYMBOLS_MAX];
    };
    double insert[HMR_SYMBOLS_MAX];
    double trans[HMR_TRANS_SIZE];
};

HMR_API void hmr_node_dump(struct hmr_node const *node, FILE *fd);

#endif
