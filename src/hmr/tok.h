#ifndef HMR_TOK_H
#define HMR_TOK_H

#include <stdbool.h>
#include <string.h>

enum hmr_tok_id
{
    HMR_TOK_NEWLINE = 0,
    HMR_TOK_WORD,
    HMR_TOK_HMM,
    HMR_TOK_COMPO,
    HMR_TOK_SLASH,
    HMR_TOK_EOF,
};

#define HMR_TOK_LINE_MAX 256

struct hmr_tok
{
    enum hmr_tok_id id;
    char line[HMR_TOK_LINE_MAX];
    unsigned line_number;
    bool consumed_line;
    char *ptr;
    char const *value;
    char *error;
};

#endif
