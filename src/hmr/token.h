#ifndef HMR_TOKEN_H
#define HMR_TOKEN_H

#include <stdbool.h>

enum hmr_token_id
{
    HMR_TOKEN_NEWLINE = 0,
    HMR_TOKEN_WORD,
    HMR_TOKEN_HMM,
    HMR_TOKEN_COMPO,
    HMR_TOKEN_SLASH,
    HMR_TOKEN_EOF,
};

#define HMR_TOKEN_LINE_MAX 256

struct hmr_token
{
    enum hmr_token_id id;
    char line[HMR_TOKEN_LINE_MAX];
    unsigned line_number;
    bool consumed_line;
    char const *value;
};

#endif
