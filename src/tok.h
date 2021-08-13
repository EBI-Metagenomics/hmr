#ifndef TOK_H
#define TOK_H

#include "hmr/rc.h"
#include <stdbool.h>
#include <stdio.h>

struct hmr_tok;

#define TOK_INIT(name) tok_init(&name)

void tok_init(struct hmr_tok *tok, char *error);
enum hmr_rc tok_next(FILE *restrict fd, struct hmr_tok *tok);

#endif
