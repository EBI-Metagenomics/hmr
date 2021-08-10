#ifndef TOKEN_H
#define TOKEN_H

#include "hmr/rc.h"
#include <stdbool.h>
#include <stdio.h>

enum token_id
{
    TOKEN_NEWLINE = 0,
    TOKEN_WORD,
    TOKEN_HMM,
    TOKEN_SLASH,
};

#define TOKEN_LINE_MAX 256

struct token
{
    enum token_id id;
    char line[TOKEN_LINE_MAX];
    char const *value;
    enum hmr_rc rc;
};

void token_init(struct token *tok);
bool token_next(FILE *restrict fd, struct token *tok);

#define TOKEN_INIT(name) token_init(&name)

#endif
