#ifndef HMR_NODE_H
#define HMR_NODE_H

#include "hmr/export.h"
#include <stdio.h>

#define HMR_SYMBOLS_MAX 32
#define HMR_TRANS_SIZE 7

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

HMR_API void hmr_node_dump(struct hmr_node const *node, FILE *restrict fd);

#endif
