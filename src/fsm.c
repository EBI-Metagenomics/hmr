#include "fsm.h"
#include "hmr.h"
#include "hmr/prof.h"
#include "node.h"
#include "prof.h"
#include "token.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct trans
{
    enum fsm_state const next;
    void (*action)(struct token const *tok, enum fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof);
};

static void nop(struct token const *token, enum fsm_state state,
                struct hmr_aux *aux, struct hmr_prof *prof);

static void header(struct token const *tok, enum fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof);

static void field_name(struct token const *tok, enum fsm_state state,
                       struct hmr_aux *aux, struct hmr_prof *prof);

static void field_content(struct token const *tok, enum fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof);

static void hmm(struct token const *tok, enum fsm_state state,
                struct hmr_aux *aux, struct hmr_prof *prof);

static void symbol(struct token const *tok, enum fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof);

static void compo(struct token const *tok, enum fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof);

static void insert(struct token const *tok, enum fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof);

static void match(struct token const *tok, enum fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof);

static void trans(struct token const *tok, enum fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc to_double(char const *str, double *val);

static struct trans const transition[][5] = {
    [FSM_BEGIN] = {[TOKEN_WORD] = {FSM_HEADER, &header},
                   [TOKEN_NEWLINE] = {FSM_ERROR, &nop},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_COMPO] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_HEADER] = {[TOKEN_WORD] = {FSM_HEADER, &header},
                    [TOKEN_NEWLINE] = {FSM_NAME, &header},
                    [TOKEN_HMM] = {FSM_ERROR, &nop},
                    [TOKEN_COMPO] = {FSM_ERROR, &nop},
                    [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_NAME] = {[TOKEN_WORD] = {FSM_CONTENT, &field_name},
                  [TOKEN_NEWLINE] = {FSM_ERROR, &nop},
                  [TOKEN_HMM] = {FSM_SYMBOL, &hmm},
                  [TOKEN_COMPO] = {FSM_ERROR, &nop},
                  [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_CONTENT] = {[TOKEN_WORD] = {FSM_CONTENT, &field_content},
                     [TOKEN_NEWLINE] = {FSM_NAME, &field_content},
                     [TOKEN_HMM] = {FSM_CONTENT, &nop},
                     [TOKEN_COMPO] = {FSM_ERROR, &nop},
                     [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_SYMBOL] = {[TOKEN_WORD] = {FSM_SYMBOL, &symbol},
                    [TOKEN_NEWLINE] = {FSM_ARROW, &symbol},
                    [TOKEN_HMM] = {FSM_ERROR, &nop},
                    [TOKEN_COMPO] = {FSM_ERROR, &nop},
                    [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_ARROW] = {[TOKEN_WORD] = {FSM_ARROW, &nop},
                   [TOKEN_NEWLINE] = {FSM_PAUSE, &nop},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_COMPO] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_COMPO] = {[TOKEN_WORD] = {FSM_COMPO, &compo},
                   [TOKEN_NEWLINE] = {FSM_INSERT, &compo},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_COMPO] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_INSERT] = {[TOKEN_WORD] = {FSM_INSERT, &insert},
                    [TOKEN_NEWLINE] = {FSM_TRANS, &insert},
                    [TOKEN_HMM] = {FSM_ERROR, &nop},
                    [TOKEN_COMPO] = {FSM_ERROR, &nop},
                    [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_MATCH] = {[TOKEN_WORD] = {FSM_MATCH, &match},
                   [TOKEN_NEWLINE] = {FSM_INSERT, &match},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_COMPO] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_TRANS] = {[TOKEN_WORD] = {FSM_TRANS, &trans},
                   [TOKEN_NEWLINE] = {FSM_PAUSE, &trans},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_COMPO] = {FSM_ERROR, &nop},
                   [TOKEN_SLASH] = {FSM_ERROR, &nop}},
    [FSM_PAUSE] = {[TOKEN_WORD] = {FSM_MATCH, &match},
                   [TOKEN_NEWLINE] = {FSM_ERROR, &nop},
                   [TOKEN_HMM] = {FSM_ERROR, &nop},
                   [TOKEN_COMPO] = {FSM_COMPO, &compo},
                   [TOKEN_SLASH] = {FSM_SLASHED, &nop}},
    [FSM_SLASHED] = {[TOKEN_WORD] = {FSM_ERROR, &nop},
                     [TOKEN_NEWLINE] = {FSM_BEGIN, &nop},
                     [TOKEN_HMM] = {FSM_ERROR, &nop},
                     [TOKEN_COMPO] = {FSM_ERROR, &nop},
                     [TOKEN_SLASH] = {FSM_ERROR, &nop}},
};

static char state_name[][10] = {
    [FSM_BEGIN] = "BEGIN",   [FSM_HEADER] = "HEADER",
    [FSM_NAME] = "NAME",     [FSM_CONTENT] = "CONTENT",
    [FSM_SYMBOL] = "SYMBOL", [FSM_ARROW] = "ARROW",
    [FSM_COMPO] = "COMPO",   [FSM_INSERT] = "INSERT",
    [FSM_MATCH] = "MATCH",   [FSM_TRANS] = "TRANS",
    [FSM_PAUSE] = "PAUSE",   [FSM_SLASHED] = "SLASHED",
    [FSM_ERROR] = "ERROR",
};

enum fsm_state fsm_next(enum fsm_state state, struct token const *tok,
                        struct hmr_aux *aux, struct hmr_prof *prof)
{
    unsigned row = (unsigned)state;
    unsigned col = (unsigned)tok->id;
    struct trans const *const t = &transition[row][col];
    t->action(tok, state, aux, prof);
    return t->next;
}

char const *fsm_name(enum fsm_state state) { return state_name[state]; }

static void nop(struct token const *token, enum fsm_state state,
                struct hmr_aux *aux, struct hmr_prof *prof)
{
    return;
}

static void header(struct token const *tok, enum fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (aux->prof.pos > aux->prof.begin + 1)
        {
            *(aux->prof.pos - 1) = ' ';
            aux->prof.pos++;
        }
        else
        {
            aux->prof.begin = prof->header;
            aux->prof.pos = aux->prof.begin + 1;
            aux->prof.end = aux->prof.begin + HMR_HEADER_MAX;
        }
        aux->prof.pos = memccpy(aux->prof.pos - 1, tok->value, '\0',
                                aux->prof.end - aux->prof.pos);
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        *(aux->prof.pos - 1) = '\0';
        hmr_aux_reset(aux);
    }
}

static void field_name(struct token const *tok, enum fsm_state state,
                       struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (!strcmp(tok->value, "NAME"))
    {
        aux->prof.begin = prof->meta.name;
        aux->prof.end = aux->prof.begin + HMR_NAME_MAX;
    }
    else if (!strcmp(tok->value, "ACC"))
    {
        aux->prof.begin = prof->meta.acc;
        aux->prof.end = aux->prof.begin + HMR_ACC_MAX;
    }
    else if (!strcmp(tok->value, "DESC"))
    {
        aux->prof.begin = prof->meta.desc;
        aux->prof.end = aux->prof.begin + HMR_DESC_MAX;
    }
    else if (!strcmp(tok->value, "LENG"))
    {
        aux->prof.begin = prof->meta.leng;
        aux->prof.end = aux->prof.begin + HMR_LENG_MAX;
    }
    else if (!strcmp(tok->value, "ALPH"))
    {
        aux->prof.begin = prof->meta.alph;
        aux->prof.end = aux->prof.begin + HMR_ALPH_MAX;
    }
    else
    {
        aux->prof.begin = NULL;
        aux->prof.pos = NULL;
        aux->prof.end = NULL;
        return;
    }
    aux->prof.pos = aux->prof.begin + 1;
}

static void field_content(struct token const *tok, enum fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (!aux->prof.begin)
        return;

    if (tok->id == TOKEN_WORD)
    {
        if (aux->prof.pos > aux->prof.begin + 1)
        {
            *(aux->prof.pos - 1) = ' ';
            aux->prof.pos++;
        }
        aux->prof.pos = memccpy(aux->prof.pos - 1, tok->value, '\0',
                                aux->prof.end - aux->prof.pos);
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        *(aux->prof.pos - 1) = '\0';
        hmr_aux_reset(aux);
    }
}

static void hmm(struct token const *tok, enum fsm_state state,
                struct hmr_aux *aux, struct hmr_prof *prof)
{
    aux->prof.begin = prof->symbols;
    aux->prof.end = aux->prof.begin + HMR_SYMBOLS_MAX;
    aux->prof.pos = aux->prof.begin + 1;
}

static void symbol(struct token const *tok, enum fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        *(aux->prof.pos - 1) = *tok->value;
        aux->prof.pos++;
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        *(aux->prof.pos - 1) = '\0';
        prof->symbols_size = (unsigned)strlen(prof->symbols);
        prof->node.symbols_size = prof->symbols_size;
        hmr_aux_reset(aux);
    }
}

static void compo(struct token const *tok, enum fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (aux->node.idx >= prof->symbols_size)
        {
            /* BUG */
            return;
        }
        to_double(tok->value, prof->node.compo + aux->node.idx++);
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        if (aux->node.idx != prof->symbols_size)
        {
            /* BUG */
            return;
        }
        hmr_aux_reset(aux);
    }
}

static void insert(struct token const *tok, enum fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (aux->node.idx >= prof->symbols_size)
        {
            /* BUG */
            return;
        }
        to_double(tok->value, prof->node.insert + aux->node.idx++);
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        if (aux->node.idx != prof->symbols_size)
        {
            /* BUG */
            return;
        }
        hmr_aux_reset(aux);
    }
}

static void match(struct token const *tok, enum fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (state == FSM_PAUSE)
        {
            unsigned i = (unsigned)strtoul(tok->value, NULL, 10);

            if (i == 0)
            {
                /* BUG */
                return;
            }
            prof->node.idx = i;
            return;
        }
        if (aux->node.idx >= prof->symbols_size)
        {
            if (aux->node.idx >= prof->symbols_size + HMR_MATCH_EXCESS_SIZE)
            {
                /* BUG */
                return;
            }
            aux->node.idx++;
            return;
        }
        to_double(tok->value, prof->node.match + aux->node.idx++);
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        if (aux->node.idx > prof->symbols_size + HMR_MATCH_EXCESS_SIZE)
        {
            /* BUG */
            return;
        }
        hmr_aux_reset(aux);
    }
}

static void trans(struct token const *tok, enum fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == TOKEN_WORD)
    {
        if (aux->node.idx >= HMR_TRANS_SIZE)
        {
            /* BUG */
            return;
        }
        to_double(tok->value, prof->node.trans + aux->node.idx++);
    }
    else if (tok->id == TOKEN_NEWLINE)
    {
        if (aux->node.idx != HMR_TRANS_SIZE)
        {
            /* BUG */
            return;
        }
        hmr_aux_reset(aux);
    }
}

static enum hmr_rc to_double(char const *str, double *val)
{
    if (str[0] == '*' && str[1] == '\0')
    {
        *val = NAN;
        return HMR_SUCCESS;
    }
    char *ptr = NULL;
    *val = strtod(str, &ptr);

    if (*val == 0.0 && str == ptr)
        return HMR_PARSEERROR;
    return HMR_SUCCESS;
}
