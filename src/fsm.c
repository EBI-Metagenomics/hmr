#include "fsm.h"
#include "aux.h"
#include "hmr/aux.h"
#include "hmr/prof.h"
#include "hmr/token.h"
#include "token.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct trans
{
    enum hmr_fsm_state const next;
    void (*action)(struct hmr_token const *tok, enum hmr_fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof);
};

static void nop(struct hmr_token const *token, enum hmr_fsm_state state,
                struct hmr_aux *aux, struct hmr_prof *prof);

static void header(struct hmr_token const *tok, enum hmr_fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof);

static void field_name(struct hmr_token const *tok, enum hmr_fsm_state state,
                       struct hmr_aux *aux, struct hmr_prof *prof);

static void field_content(struct hmr_token const *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof);

static void hmm(struct hmr_token const *tok, enum hmr_fsm_state state,
                struct hmr_aux *aux, struct hmr_prof *prof);

static void symbol(struct hmr_token const *tok, enum hmr_fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof);

static void compo(struct hmr_token const *tok, enum hmr_fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof);

static void insert(struct hmr_token const *tok, enum hmr_fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof);

static void match(struct hmr_token const *tok, enum hmr_fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof);

static void trans(struct hmr_token const *tok, enum hmr_fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc to_double(char const *str, double *val);

static struct trans const transition[][5] = {
    [HMR_FSM_BEGIN] = {[HMR_TOKEN_WORD] = {HMR_FSM_HEADER, &header},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_HEADER] = {[HMR_TOKEN_WORD] = {HMR_FSM_HEADER, &header},
                        [HMR_TOKEN_NEWLINE] = {HMR_FSM_NAME, &header},
                        [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_NAME] = {[HMR_TOKEN_WORD] = {HMR_FSM_CONTENT, &field_name},
                      [HMR_TOKEN_NEWLINE] = {HMR_FSM_ERROR, &nop},
                      [HMR_TOKEN_HMM] = {HMR_FSM_SYMBOL, &hmm},
                      [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                      [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_CONTENT] = {[HMR_TOKEN_WORD] = {HMR_FSM_CONTENT, &field_content},
                         [HMR_TOKEN_NEWLINE] = {HMR_FSM_NAME, &field_content},
                         [HMR_TOKEN_HMM] = {HMR_FSM_CONTENT, &nop},
                         [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_SYMBOL] = {[HMR_TOKEN_WORD] = {HMR_FSM_SYMBOL, &symbol},
                        [HMR_TOKEN_NEWLINE] = {HMR_FSM_ARROW, &symbol},
                        [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_ARROW] = {[HMR_TOKEN_WORD] = {HMR_FSM_ARROW, &nop},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_PAUSE, &nop},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_COMPO] = {[HMR_TOKEN_WORD] = {HMR_FSM_COMPO, &compo},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_INSERT, &compo},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_INSERT] = {[HMR_TOKEN_WORD] = {HMR_FSM_INSERT, &insert},
                        [HMR_TOKEN_NEWLINE] = {HMR_FSM_TRANS, &insert},
                        [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_MATCH] = {[HMR_TOKEN_WORD] = {HMR_FSM_MATCH, &match},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_INSERT, &match},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_TRANS] = {[HMR_TOKEN_WORD] = {HMR_FSM_TRANS, &trans},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_PAUSE, &trans},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
    [HMR_FSM_PAUSE] = {[HMR_TOKEN_WORD] = {HMR_FSM_MATCH, &match},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_COMPO, &compo},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_SLASHED, &nop}},
    [HMR_FSM_SLASHED] = {[HMR_TOKEN_WORD] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_NEWLINE] = {HMR_FSM_BEGIN, &nop},
                         [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop}},
};

static char state_name[][10] = {
    [HMR_FSM_BEGIN] = "BEGIN",   [HMR_FSM_HEADER] = "HEADER",
    [HMR_FSM_NAME] = "NAME",     [HMR_FSM_CONTENT] = "CONTENT",
    [HMR_FSM_SYMBOL] = "SYMBOL", [HMR_FSM_ARROW] = "ARROW",
    [HMR_FSM_COMPO] = "COMPO",   [HMR_FSM_INSERT] = "INSERT",
    [HMR_FSM_MATCH] = "MATCH",   [HMR_FSM_TRANS] = "TRANS",
    [HMR_FSM_PAUSE] = "PAUSE",   [HMR_FSM_SLASHED] = "SLASHED",
    [HMR_FSM_ERROR] = "ERROR",
};

enum hmr_fsm_state fsm_next(enum hmr_fsm_state state,
                            struct hmr_token const *tok, struct hmr_aux *aux,
                            struct hmr_prof *prof)
{
    unsigned row = (unsigned)state;
    unsigned col = (unsigned)tok->id;
    struct trans const *const t = &transition[row][col];
    t->action(tok, state, aux, prof);
    return t->next;
}

char const *fsm_name(enum hmr_fsm_state state) { return state_name[state]; }

static void nop(struct hmr_token const *token, enum hmr_fsm_state state,
                struct hmr_aux *aux, struct hmr_prof *prof)
{
    return;
}

static void header(struct hmr_token const *tok, enum hmr_fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == HMR_TOKEN_WORD)
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
    else if (tok->id == HMR_TOKEN_NEWLINE)
    {
        *(aux->prof.pos - 1) = '\0';
        aux_reset(aux);
    }
}

static void field_name(struct hmr_token const *tok, enum hmr_fsm_state state,
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

static void field_content(struct hmr_token const *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (!aux->prof.begin)
        return;

    if (tok->id == HMR_TOKEN_WORD)
    {
        if (aux->prof.pos > aux->prof.begin + 1)
        {
            *(aux->prof.pos - 1) = ' ';
            aux->prof.pos++;
        }
        aux->prof.pos = memccpy(aux->prof.pos - 1, tok->value, '\0',
                                aux->prof.end - aux->prof.pos);
    }
    else if (tok->id == HMR_TOKEN_NEWLINE)
    {
        *(aux->prof.pos - 1) = '\0';
        aux_reset(aux);
    }
}

static void hmm(struct hmr_token const *tok, enum hmr_fsm_state state,
                struct hmr_aux *aux, struct hmr_prof *prof)
{
    aux->prof.begin = prof->symbols;
    aux->prof.end = aux->prof.begin + HMR_SYMBOLS_MAX;
    aux->prof.pos = aux->prof.begin + 1;
}

static void symbol(struct hmr_token const *tok, enum hmr_fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == HMR_TOKEN_WORD)
    {
        *(aux->prof.pos - 1) = *tok->value;
        aux->prof.pos++;
    }
    else if (tok->id == HMR_TOKEN_NEWLINE)
    {
        *(aux->prof.pos - 1) = '\0';
        prof->symbols_size = (unsigned)strlen(prof->symbols);
        prof->node.symbols_size = prof->symbols_size;
        aux_reset(aux);
    }
}

static void compo(struct hmr_token const *tok, enum hmr_fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == HMR_TOKEN_WORD)
    {
        if (aux->node.idx >= prof->symbols_size)
        {
            /* BUG */
            return;
        }
        to_double(tok->value, prof->node.compo + aux->node.idx++);
    }
    else if (tok->id == HMR_TOKEN_NEWLINE)
    {
        if (aux->node.idx != prof->symbols_size)
        {
            /* BUG */
            return;
        }
        aux_reset(aux);
    }
}

static void insert(struct hmr_token const *tok, enum hmr_fsm_state state,
                   struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == HMR_TOKEN_WORD)
    {
        if (aux->node.idx >= prof->symbols_size)
        {
            /* BUG */
            return;
        }
        to_double(tok->value, prof->node.insert + aux->node.idx++);
    }
    else if (tok->id == HMR_TOKEN_NEWLINE)
    {
        if (aux->node.idx != prof->symbols_size)
        {
            /* BUG */
            return;
        }
        aux_reset(aux);
    }
}

static void match(struct hmr_token const *tok, enum hmr_fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == HMR_TOKEN_WORD)
    {
        if (state == HMR_FSM_PAUSE)
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
    else if (tok->id == HMR_TOKEN_NEWLINE)
    {
        if (aux->node.idx > prof->symbols_size + HMR_MATCH_EXCESS_SIZE)
        {
            /* BUG */
            return;
        }
        aux_reset(aux);
    }
}

static void trans(struct hmr_token const *tok, enum hmr_fsm_state state,
                  struct hmr_aux *aux, struct hmr_prof *prof)
{
    if (tok->id == HMR_TOKEN_WORD)
    {
        if (aux->node.idx >= HMR_TRANS_SIZE)
        {
            /* BUG */
            return;
        }
        to_double(tok->value, prof->node.trans + aux->node.idx++);
    }
    else if (tok->id == HMR_TOKEN_NEWLINE)
    {
        if (aux->node.idx != HMR_TRANS_SIZE)
        {
            /* BUG */
            return;
        }
        aux_reset(aux);
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
