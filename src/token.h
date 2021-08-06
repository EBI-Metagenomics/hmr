#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stdio.h>

enum token_id
{
    NEWLINE,
    WORD,
    HMM,
    SLASH,
};

#define TOKEN_LINE_MAX 256

struct token
{
    enum token_id id;
    struct
    {
        char data[TOKEN_LINE_MAX];
        char const *begin;
        char const *end;
    } line;
    unsigned len;
};

void token_init(struct token *token);
bool token_next(FILE *restrict fd, struct token *t, int *rc);

#define TOKEN_DECL(name)                                                       \
    struct token name;                                                         \
    token_init(&name);

#endif
