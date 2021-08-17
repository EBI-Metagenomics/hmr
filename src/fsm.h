#ifndef FSM_H
#define FSM_H

#include "hmr/state.h"

struct hmr_aux;
struct hmr_prof;
struct hmr_tok;

static inline void fsm_init(enum hmr_state *state) { *state = HMR_FSM_BEGIN; }

enum hmr_state fsm_next(enum hmr_state state, struct hmr_tok *tok,
                        struct hmr_aux *aux, struct hmr_prof *prof);

char const *fsm_name(enum hmr_state state);

#endif
