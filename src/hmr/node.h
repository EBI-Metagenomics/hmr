#ifndef HMR_NODE_H
#define HMR_NODE_H

#define HMR_HEADER_MAX 64
#define HMR_NAME_MAX 64
#define HMR_ACC_MAX 64
#define HMR_DESC_MAX 128
#define HMR_LENG_MAX 8
#define HMR_ALPH_MAX 12
#define HMR_SYMBOLS_MAX 32
#define HMR_TRANS_SIZE 7

struct hmr_node
{
    union
    {
        double compo[HMR_SYMBOLS_MAX];
        double match[HMR_SYMBOLS_MAX];
    };
    double insert[HMR_SYMBOLS_MAX];
    double trans[HMR_TRANS_SIZE];

    struct
    {
        unsigned idx;
        double *begin;
        double *pos;
        double *end;
    } __;
};

#endif
