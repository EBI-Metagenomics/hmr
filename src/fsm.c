#include "fsm.h"
#include <stdio.h>

struct trans
{
    enum fsm_state const next;
    void (*action)(struct token const *, enum fsm_state);
};

static void nop(struct token const *token, enum fsm_state state);

static struct trans const trans[][4] = {
    [BEGIN] = {[TOKEN_WORD] = {HEADER, &nop},
               [TOKEN_NEWLINE] = {BEGIN, &nop},
               [TOKEN_HMM] = {ERROR, &nop},
               [TOKEN_SLASH] = {ERROR, &nop}},
    [HEADER] = {[TOKEN_WORD] = {HEADER, &nop},
                [TOKEN_NEWLINE] = {NAME, &nop},
                [TOKEN_HMM] = {ERROR, &nop},
                [TOKEN_SLASH] = {ERROR, &nop}},
    [NAME] = {[TOKEN_WORD] = {CONTENT, &nop},
              [TOKEN_NEWLINE] = {ERROR, &nop},
              [TOKEN_HMM] = {ABC, &nop},
              [TOKEN_SLASH] = {ERROR, &nop}},
    [CONTENT] = {[TOKEN_WORD] = {CONTENT, &nop},
                 [TOKEN_NEWLINE] = {NAME, &nop},
                 [TOKEN_HMM] = {CONTENT, &nop},
                 [TOKEN_SLASH] = {ERROR, &nop}},
    [ABC] = {[TOKEN_WORD] = {ABC, &nop},
             [TOKEN_NEWLINE] = {ARROW, &nop},
             [TOKEN_HMM] = {ABC, &nop},
             [TOKEN_SLASH] = {ERROR, &nop}},
    [ARROW] = {[TOKEN_WORD] = {ARROW, &nop},
               [TOKEN_NEWLINE] = {COMPO, &nop},
               [TOKEN_HMM] = {ERROR, &nop},
               [TOKEN_SLASH] = {ERROR, &nop}},
    [COMPO] = {[TOKEN_WORD] = {COMPO, &nop},
               [TOKEN_NEWLINE] = {INSERT, &nop},
               [TOKEN_HMM] = {ERROR, &nop},
               [TOKEN_SLASH] = {ERROR, &nop}},
    [INSERT] = {[TOKEN_WORD] = {INSERT, &nop},
                [TOKEN_NEWLINE] = {TRANS, &nop},
                [TOKEN_HMM] = {ERROR, &nop},
                [TOKEN_SLASH] = {ERROR, &nop}},
    [TRANS] = {[TOKEN_WORD] = {TRANS, &nop},
               [TOKEN_NEWLINE] = {MATCH, &nop},
               [TOKEN_HMM] = {ERROR, &nop},
               [TOKEN_SLASH] = {ERROR, &nop}},
    [MATCH] = {[TOKEN_WORD] = {MATCH, &nop},
               [TOKEN_NEWLINE] = {INSERT, &nop},
               [TOKEN_HMM] = {ERROR, &nop},
               [TOKEN_SLASH] = {BEGIN, &nop}},
};

static char const *const state_name[] = {
    [BEGIN] = "BEGIN",     [HEADER] = "HEADER", [NAME] = "NAME",
    [CONTENT] = "CONTENT", [ABC] = "ABC",       [ARROW] = "ARROW",
    [COMPO] = "COMPO",     [INSERT] = "INSERT", [TRANS] = "TRANS",
    [MATCH] = "MATCH",     [ERROR] = "ERROR"};

enum fsm_state fsm_next(enum fsm_state state, struct token const *token)
{
    unsigned row = (unsigned)state;
    unsigned col = (unsigned)token->id;
    struct trans const *const t = &trans[row][col];
    t->action(token, state);
    return t->next;
}

static void nop(struct token const *token, enum fsm_state state)
{
    printf("State: %s ", state_name[state]);
    if (*token->line.begin == '\n')
        printf("Token: TOKEN_NEWLINE:[\\n]\n");
    else
    {
        if (token->id == TOKEN_WORD)
            printf("Token: TOKEN_WORD:[%.*s]\n", token->len, token->line.begin);
        else if (token->id == TOKEN_HMM)
            printf("Token: TOKEN_HMM:[%.*s]\n", token->len, token->line.begin);
        else if (token->id == TOKEN_SLASH)
            printf("Token: TOKEN_SLASH:[%.*s]\n", token->len,
                   token->line.begin);
    }
}
