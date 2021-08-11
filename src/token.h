#ifndef TOKEN_H
#define TOKEN_H

#include "hmr/rc.h"
#include <stdbool.h>
#include <stdio.h>

struct hmr_token;

#define TOKEN_INIT(name) token_init(&name)

void token_init(struct hmr_token *tok);
enum hmr_rc token_next(FILE *restrict fd, struct hmr_token *tok);

#endif
