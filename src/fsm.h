#ifndef FSM_H
#define FSM_H

enum fsm_state
{
    FSM_BEGIN = 0,
    FSM_HEADER,
    FSM_NAME,
    FSM_CONTENT,
    FSM_SYMBOL,
    FSM_ARROW,
    FSM_COMPO,
    FSM_INSERT,
    FSM_MATCH,
    FSM_TRANS,
    FSM_PAUSE,
    FSM_SLASHED,
    FSM_ERROR
};

struct hmr_prof;
struct token;

static inline void fsm_init(enum fsm_state *state) { *state = FSM_HEADER; }

enum fsm_state fsm_next(enum fsm_state state, struct token const *token,
                        struct hmr_prof *prof);

#endif
