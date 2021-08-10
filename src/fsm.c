#include "fsm.h"
#include "hmr/prof.h"
#include "node.h"
#include "prof.h"
#include "token.h"
#include <stdlib.h>
#include <string.h>

struct trans
{
    enum fsm_state const next;
    void (*action)(struct token const *tok, enum fsm_state state,
                   struct hmr_prof *prof);
};

static void nop(struct token const *token, enum fsm_state state,
                struct hmr_prof *prof);

static void header(struct token const *tok, enum fsm_state state,
                   struct hmr_prof *prof);

static void field_name(struct token const *tok, enum fsm_state state,
                       struct hmr_prof *prof);

static void field_content(struct token const *tok, enum fsm_state state,
                          struct hmr_prof *prof);

static void hmm(struct token const *tok, enum fsm_state state,
                struct hmr_prof *prof);

static void symbol(struct token const *tok, enum fsm_state state,
                   struct hmr_prof *prof);

static void arrow(struct token const *tok, enum fsm_state state,
                  struct hmr_prof *prof);

static void compo(struct token const *tok, enum fsm_state state,
                  struct hmr_prof *prof);

static void insert(struct token const *tok, enum fsm_state state,
                   struct hmr_prof *prof);

static void match(struct token const *tok, enum fsm_state state,
                  struct hmr_prof *prof);

static void trans(struct token const *tok, enum fsm_state state,
                  struct hmr_prof *prof);

static struct trans const transition[][4] = {
    [FSM_BEGIN] = {[TOKEN_WORD] = {FSM_HEADER, &header},
                   [TOKEN_NEWLINE] = {FSM_ERROR, &nop},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_HEADER] = {[TOKEN_WORD] = {FSM_HEADER, &header},
                    [TOKEN_NEWLINE] = {FSM_NAME, &header},
                    [TOKEN_HMM] = {FSM_ERROR, &nop},
                    [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_NAME] = {[TOKEN_WORD] = {FSM_CONTENT, &field_name},
                  [TOKEN_NEWLINE] = {FSM_ERROR, &nop},
                  [TOKEN_HMM] = {FSM_SYMBOL, &hmm},
                  [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_CONTENT] = {[TOKEN_WORD] = {FSM_CONTENT, &field_content},
                     [TOKEN_NEWLINE] = {FSM_NAME, &field_content},
                     [TOKEN_HMM] = {FSM_CONTENT, &nop},
                     [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_SYMBOL] = {[TOKEN_WORD] = {FSM_SYMBOL, &symbol},
                    [TOKEN_NEWLINE] = {FSM_ARROW, &symbol},
                    [TOKEN_HMM] = {FSM_ERROR, &nop},
                    [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_ARROW] = {[TOKEN_WORD] = {FSM_ARROW, &arrow},
                   [TOKEN_NEWLINE] = {FSM_COMPO, &arrow},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_COMPO] = {[TOKEN_WORD] = {FSM_COMPO, &compo},
                   [TOKEN_NEWLINE] = {FSM_INSERT, &compo},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_INSERT] = {[TOKEN_WORD] = {FSM_INSERT, &insert},
                    [TOKEN_NEWLINE] = {FSM_TRANS, &insert},
                    [TOKEN_HMM] = {FSM_ERROR, &nop},
                    [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_MATCH] = {[TOKEN_WORD] = {FSM_MATCH, &match},
                   [TOKEN_NEWLINE] = {FSM_INSERT, &match},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_TRANS] = {[TOKEN_WORD] = {FSM_TRANS, &trans},
                   [TOKEN_NEWLINE] = {FSM_PAUSE, &trans},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_PAUSE] = {[TOKEN_WORD] = {FSM_MATCH, &match},
                   [TOKEN_NEWLINE] = {FSM_ERROR, &nop},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_SLASHED, &nop}},
    [FSM_SLASHED] = {[TOKEN_WORD] = {FSM_ERROR, &nop},
                     [TOKEN_NEWLINE] = {FSM_BEGIN, &nop},
                     [TOKEN_HMM] = {FSM_ERROR, &nop},
                     [TOKEN_SLASH] = {FSM_ERROR, &nop}},
};

static char const *const state_name[] = {
    [FSM_BEGIN] = "BEGIN",   [FSM_HEADER] = "HEADER",
    [FSM_NAME] = "NAME",     [FSM_CONTENT] = "CONTENT",
    [FSM_SYMBOL] = "SYMBOL", [FSM_ARROW] = "ARROW",
    [FSM_COMPO] = "COMPO",   [FSM_INSERT] = "INSERT",
    [FSM_MATCH] = "MATCH",   [FSM_TRANS] = "TRANS",
    [FSM_PAUSE] = "PAUSE",   [FSM_SLASHED] = "SLASHED",
    [FSM_ERROR] = "ERROR"};

enum fsm_state fsm_next(enum fsm_state state, struct token const *tok,
                        struct hmr_prof *prof)
{
    unsigned row = (unsigned)state;
    unsigned col = (unsigned)tok->id;
    struct trans const *const t = &transition[row][col];
    t->action(tok, state, prof);
    return t->next;
}

static void nop(struct token const *token, enum fsm_state state,
                struct hmr_prof *prof)
{
    if (token->id == TOKEN_SLASH)
    {
        printf("SLASH token\n");
        return;
    }
    printf("State: %s ", state_name[state]);
    if (*token->value == '\n')
        printf("Token: TOKEN_NEWLINE:[\\n]\n");
    else
    {
        if (token->id == TOKEN_WORD)
            printf("Token: TOKEN_WORD:[%s]\n", token->value);
        else if (token->id == TOKEN_HMM)
            printf("Token: TOKEN_HMM:[%s]\n", token->value);
        else if (token->id == TOKEN_SLASH)
            printf("Token: TOKEN_SLASH:[%s]\n", token->value);
    }
}

static void header(struct token const *tok, enum fsm_state state,
                   struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (prof->__.pos > prof->__.begin + 1)
        {
            *(prof->__.pos - 1) = ' ';
            prof->__.pos++;
        }
        else
        {
            prof->__.begin = prof->header;
            prof->__.pos = prof->__.begin + 1;
            prof->__.end = prof->__.begin + HMR_HEADER_MAX;
        }
        prof->__.pos = memccpy(prof->__.pos - 1, tok->value, '\0',
                               prof->__.end - prof->__.pos);
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        *(prof->__.pos - 1) = '\0';
        prof_reset_tmp(prof);
    }
}

static void field_name(struct token const *tok, enum fsm_state state,
                       struct hmr_prof *prof)
{
    if (!strcmp(tok->value, "NAME"))
    {
        prof->__.begin = prof->meta.NAME;
        prof->__.end = prof->__.begin + HMR_NAME_MAX;
    }
    else if (!strcmp(tok->value, "ACC"))
    {
        prof->__.begin = prof->meta.ACC;
        prof->__.end = prof->__.begin + HMR_ACC_MAX;
    }
    else if (!strcmp(tok->value, "DESC"))
    {
        prof->__.begin = prof->meta.DESC;
        prof->__.end = prof->__.begin + HMR_DESC_MAX;
    }
    else if (!strcmp(tok->value, "LENG"))
    {
        prof->__.begin = prof->meta.LENG;
        prof->__.end = prof->__.begin + HMR_LENG_MAX;
    }
    else if (!strcmp(tok->value, "ALPH"))
    {
        prof->__.begin = prof->meta.ALPH;
        prof->__.end = prof->__.begin + HMR_ALPH_MAX;
    }
    else
    {
        prof->__.begin = NULL;
        prof->__.pos = NULL;
        prof->__.end = NULL;
        return;
    }
    prof->__.pos = prof->__.begin + 1;
}

static void field_content(struct token const *tok, enum fsm_state state,
                          struct hmr_prof *prof)
{
    if (!prof->__.begin)
        return;

    if (tok->id == TOKEN_WORD)
    {
        if (prof->__.pos > prof->__.begin + 1)
        {
            *(prof->__.pos - 1) = ' ';
            prof->__.pos++;
        }
        prof->__.pos = memccpy(prof->__.pos - 1, tok->value, '\0',
                               prof->__.end - prof->__.pos);
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        *(prof->__.pos - 1) = '\0';
        prof_reset_tmp(prof);
    }
}

static void hmm(struct token const *tok, enum fsm_state state,
                struct hmr_prof *prof)
{
    prof->__.begin = prof->symbols;
    prof->__.end = prof->__.begin + HMR_SYMBOLS_MAX;
    prof->__.pos = prof->__.begin + 1;
}

static void symbol(struct token const *tok, enum fsm_state state,
                   struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        *(prof->__.pos - 1) = *tok->value;
        prof->__.pos++;
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        *(prof->__.pos - 1) = '\0';
        prof->symbols_size = (unsigned)strlen(prof->symbols);
        prof_reset_tmp(prof);
    }
}

static void arrow(struct token const *tok, enum fsm_state state,
                  struct hmr_prof *prof)
{
}

static inline int to_double(char const *begin, unsigned len, double *val)
{
    if (len > 7)
        return HMR_ILLEGALARG;

    char data[8];
    memcpy(data, begin, len);
    data[len] = '\0';

    char *ptr = NULL;
    *val = strtod(data, &ptr);

    if (*val == 0.0 && data == ptr)
        return HMR_PARSEERROR;

    return HMR_SUCCESS;
}

static void compo(struct token const *tok, enum fsm_state state,
                  struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (tok->value[0] == 'C')
        {
            if (strcmp(tok->value, "COMPO"))
            {
                /* BUG */
                return;
            }
            prof->node.__.idx = 0;
            return;
        }
        if (prof->node.__.idx >= prof->symbols_size)
        {
            /* BUG */
        }

        /* *(prof->__.pos - 1) = *tok->value; */
        /* prof->__.pos++; */
        char *ptr = NULL;
        double val = strtod(tok->value, &ptr);

        if (val == 0.0 && tok->value == ptr)
        {
            /* BUG */
            return;
            /* return HMR_PARSEERROR; */
        }
        prof->node.compo[prof->node.__.idx++] = val;
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        if (prof->node.__.idx != prof->symbols_size)
        {
            /* BUG */
        }
        node_reset_tmp(&prof->node);
    }
}

static void insert(struct token const *tok, enum fsm_state state,
                   struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (prof->node.__.idx >= prof->symbols_size)
        {
            /* BUG */
        }

        char *ptr = NULL;
        double val = strtod(tok->value, &ptr);

        if (val == 0.0 && tok->value == ptr)
        {
            /* BUG */
            return;
            /* return HMR_PARSEERROR; */
        }
        prof->node.insert[prof->node.__.idx++] = val;
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        if (prof->node.__.idx != prof->symbols_size)
        {
            /* BUG */
        }
        node_reset_tmp(&prof->node);
    }
}

static void match(struct token const *tok, enum fsm_state state,
                  struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (prof->node.__.idx == 0)
        {
            unsigned node_idx = (unsigned)strtoul(tok->value, NULL, 10);

            if (node_idx == 0)
            {
                /* BUG */
            }
            printf("Node: %d\n", node_idx);
            prof->node.__.idx++;
            return;
        }
        if (prof->node.__.idx > prof->symbols_size)
        {
            /* BUG */
        }

        char *ptr = NULL;
        double val = strtod(tok->value, &ptr);

        if (val == 0.0 && tok->value == ptr)
        {
            /* BUG */
            return;
            /* return HMR_PARSEERROR; */
        }
        prof->node.match[prof->node.__.idx - 1] = val;
        prof->node.__.idx++;
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        if (prof->node.__.idx != prof->symbols_size + 1)
        {
            /* BUG */
        }
        node_reset_tmp(&prof->node);
    }
}

static void trans(struct token const *tok, enum fsm_state state,
                  struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (prof->node.__.idx >= HMR_TRANS_SIZE)
        {
            /* BUG */
        }

        char *ptr = NULL;
        double val = strtod(tok->value, &ptr);

        if (val == 0.0 && tok->value == ptr)
        {
            /* BUG */
            return;
            /* return HMR_PARSEERROR; */
        }
        prof->node.trans[prof->node.__.idx++] = val;
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        if (prof->node.__.idx != HMR_TRANS_SIZE)
        {
            /* BUG */
        }
        node_reset_tmp(&prof->node);
    }
}
