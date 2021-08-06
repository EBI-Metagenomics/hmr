#include "fsm.h"
#include <stdio.h>

static char const *const name[] = {
    [BEGIN] = "BEGIN",     [HEADER] = "HEADER", [NAME] = "NAME",
    [CONTENT] = "CONTENT", [ABC] = "ABC",       [ARROW] = "ARROW",
    [COMPO] = "COMPO",     [INSERT] = "INSERT", [TRANS] = "TRANS",
    [MATCH] = "MATCH",     [ERROR] = "ERROR"};

struct branch
{
    enum fsm_state const next;
    void (*action)(struct token const *, enum fsm_state);
};

static void nop(struct token const *token, enum fsm_state state)
{
    printf("State: %s ", name[state]);
    if (*token->line.begin == '\n')
        printf("Token: NEWLINE:[\\n]\n");
    else
    {
        if (token->id == WORD)
            printf("Token: WORD:[%.*s]\n", token->len, token->line.begin);
        else if (token->id == HMM)
            printf("Token: HMM:[%.*s]\n", token->len, token->line.begin);
        else if (token->id == SLASH)
            printf("Token: SLASH:[%.*s]\n", token->len, token->line.begin);
    }
}

struct branch const transitions[][4] = {
    [BEGIN] = {[WORD] = {HEADER, &nop},
               [NEWLINE] = {BEGIN, &nop},
               [HMM] = {ERROR, &nop},
               [SLASH] = {ERROR, &nop}},
    [HEADER] = {[WORD] = {HEADER, &nop},
                [NEWLINE] = {NAME, &nop},
                [HMM] = {ERROR, &nop},
                [SLASH] = {ERROR, &nop}},
    [NAME] = {[WORD] = {CONTENT, &nop},
              [NEWLINE] = {ERROR, &nop},
              [HMM] = {ABC, &nop},
              [SLASH] = {ERROR, &nop}},
    [CONTENT] = {[WORD] = {CONTENT, &nop},
                 [NEWLINE] = {NAME, &nop},
                 [HMM] = {CONTENT, &nop},
                 [SLASH] = {ERROR, &nop}},
    [ABC] = {[WORD] = {ABC, &nop},
             [NEWLINE] = {ARROW, &nop},
             [HMM] = {ABC, &nop},
             [SLASH] = {ERROR, &nop}},
    [ARROW] = {[WORD] = {ARROW, &nop},
               [NEWLINE] = {COMPO, &nop},
               [HMM] = {ERROR, &nop},
               [SLASH] = {ERROR, &nop}},
    [COMPO] = {[WORD] = {COMPO, &nop},
               [NEWLINE] = {INSERT, &nop},
               [HMM] = {ERROR, &nop},
               [SLASH] = {ERROR, &nop}},
    [INSERT] = {[WORD] = {INSERT, &nop},
                [NEWLINE] = {TRANS, &nop},
                [HMM] = {ERROR, &nop},
                [SLASH] = {ERROR, &nop}},
    [TRANS] = {[WORD] = {TRANS, &nop},
               [NEWLINE] = {MATCH, &nop},
               [HMM] = {ERROR, &nop},
               [SLASH] = {ERROR, &nop}},
    [MATCH] = {[WORD] = {MATCH, &nop},
               [NEWLINE] = {INSERT, &nop},
               [HMM] = {ERROR, &nop},
               [SLASH] = {BEGIN, &nop}},
};

enum fsm_state fsm_next(enum fsm_state state, struct token const *token)
{
    if (state == ABC)
    {
        printf("");
    }
    int row = (int)state;
    int col = (int)token->id;
    struct branch const *const b = &transitions[row][col];
    b->action(token, state);
    return b->next;
}
