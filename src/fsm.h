#ifndef FSM_H
#define FSM_H

#include "token.h"

enum fsm_state
{
    BEGIN = 0,
    HEADER,
    NAME,
    CONTENT,
    ABC,
    ARROW,
    COMPO,
    INSERT,
    TRANS,
    MATCH,
    ERROR
};

enum fsm_state fsm_next(enum fsm_state state, struct token const *token);

#endif
