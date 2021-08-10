#ifndef PROF_H
#define PROF_H

#include "hmr/rc.h"
#include <stdio.h>

enum fsm_state;
struct hmr_prof;
struct token;

enum hmr_rc prof_next(struct hmr_prof *prof, FILE *restrict fd,
                      enum fsm_state *state, struct token *tok);

void prof_reset_tmp(struct hmr_prof *prof);

#endif
