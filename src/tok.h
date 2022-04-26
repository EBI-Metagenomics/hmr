#ifndef TOK_H
#define TOK_H

#include "hmr/rc.h"
#include <stdio.h>

struct hmr_tok;

void tok_init(struct hmr_tok *tok, char *error);
enum hmr_rc tok_next(struct hmr_tok *tok, FILE *restrict fp);

#endif
