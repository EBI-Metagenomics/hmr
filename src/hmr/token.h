#ifndef HMR_TOKEN_H
#define HMR_TOKEN_H

#include "hmr/rc.h"

enum hmr_token_id
{
    HMR_TOKEN_NEWLINE = 0,
    HMR_TOKEN_WORD,
    HMR_TOKEN_HMM,
    HMR_TOKEN_COMPO,
    HMR_TOKEN_SLASH,
};

#define HMR_TOKEN_LINE_MAX 256

struct hmr_token
{
    enum hmr_token_id id;
    char line[HMR_TOKEN_LINE_MAX];
    char const *value;
    enum hmr_rc rc;
};

#endif
