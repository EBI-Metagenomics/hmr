#ifndef FSM_H
#define FSM_H

#include "hmr/fsm.h"

struct hmr_aux;
struct hmr_prof;
struct hmr_token;

static inline void fsm_init(enum hmr_fsm_state *state)
{
    *state = HMR_FSM_BEGIN;
}

enum hmr_fsm_state fsm_next(enum hmr_fsm_state state, struct hmr_token *token,
                            struct hmr_aux *aux, struct hmr_prof *prof);

char const *fsm_name(enum hmr_fsm_state state);

#endif
