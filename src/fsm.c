#include "fsm.h"
#include "aux.h"
#include "bug.h"
#include "error.h"
#include "hmr/aux.h"
#include "hmr/prof.h"
#include "hmr/token.h"
#include "token.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define XSTR(s) STR(s)
#define STR(s) #s

#define DEC_ERROR "failed to parse decimal number"

struct trans
{
    enum hmr_fsm_state const next;
    enum hmr_rc (*action)(struct hmr_token *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof);
};

static enum hmr_rc nop(struct hmr_token *token, enum hmr_fsm_state state,
                       struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc unexpected_eof(struct hmr_token *token,
                                  enum hmr_fsm_state state, struct hmr_aux *aux,
                                  struct hmr_prof *prof);

static enum hmr_rc header(struct hmr_token *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc field_name(struct hmr_token *tok, enum hmr_fsm_state state,
                              struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc field_content(struct hmr_token *tok,
                                 enum hmr_fsm_state state, struct hmr_aux *aux,
                                 struct hmr_prof *prof);

static enum hmr_rc hmm(struct hmr_token *tok, enum hmr_fsm_state state,
                       struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc symbol(struct hmr_token *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc compo(struct hmr_token *tok, enum hmr_fsm_state state,
                         struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc insert(struct hmr_token *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc match(struct hmr_token *tok, enum hmr_fsm_state state,
                         struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc trans(struct hmr_token *tok, enum hmr_fsm_state state,
                         struct hmr_aux *aux, struct hmr_prof *prof);

static enum hmr_rc to_double(char const *str, double *val);

static enum hmr_rc check_required_metadata(struct hmr_prof *prof);

static struct trans const transition[][6] = {
    [HMR_FSM_BEGIN] = {[HMR_TOKEN_WORD] = {HMR_FSM_HEADER, &header},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_EOF] = {HMR_FSM_END, &nop}},
    [HMR_FSM_HEADER] = {[HMR_TOKEN_WORD] = {HMR_FSM_HEADER, &header},
                        [HMR_TOKEN_NEWLINE] = {HMR_FSM_NAME, &header},
                        [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_NAME] = {[HMR_TOKEN_WORD] = {HMR_FSM_CONTENT, &field_name},
                      [HMR_TOKEN_NEWLINE] = {HMR_FSM_ERROR, &nop},
                      [HMR_TOKEN_HMM] = {HMR_FSM_SYMBOL, &hmm},
                      [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                      [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                      [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_CONTENT] = {[HMR_TOKEN_WORD] = {HMR_FSM_CONTENT, &field_content},
                         [HMR_TOKEN_NEWLINE] = {HMR_FSM_NAME, &field_content},
                         [HMR_TOKEN_HMM] = {HMR_FSM_CONTENT, &nop},
                         [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_SYMBOL] = {[HMR_TOKEN_WORD] = {HMR_FSM_SYMBOL, &symbol},
                        [HMR_TOKEN_NEWLINE] = {HMR_FSM_ARROW, &symbol},
                        [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_ARROW] = {[HMR_TOKEN_WORD] = {HMR_FSM_ARROW, &nop},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_PAUSE, &nop},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_COMPO] = {[HMR_TOKEN_WORD] = {HMR_FSM_COMPO, &compo},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_INSERT, &compo},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_INSERT] = {[HMR_TOKEN_WORD] = {HMR_FSM_INSERT, &insert},
                        [HMR_TOKEN_NEWLINE] = {HMR_FSM_TRANS, &insert},
                        [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                        [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_MATCH] = {[HMR_TOKEN_WORD] = {HMR_FSM_MATCH, &match},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_INSERT, &match},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_TRANS] = {[HMR_TOKEN_WORD] = {HMR_FSM_TRANS, &trans},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_PAUSE, &trans},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_PAUSE] = {[HMR_TOKEN_WORD] = {HMR_FSM_MATCH, &match},
                       [HMR_TOKEN_NEWLINE] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                       [HMR_TOKEN_COMPO] = {HMR_FSM_COMPO, &nop},
                       [HMR_TOKEN_SLASH] = {HMR_FSM_SLASHED, &nop},
                       [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
    [HMR_FSM_SLASHED] = {[HMR_TOKEN_WORD] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_NEWLINE] = {HMR_FSM_BEGIN, &nop},
                         [HMR_TOKEN_HMM] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_COMPO] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_SLASH] = {HMR_FSM_ERROR, &nop},
                         [HMR_TOKEN_EOF] = {HMR_FSM_ERROR, &unexpected_eof}},
};

static char state_name[][10] = {
    [HMR_FSM_BEGIN] = "BEGIN",   [HMR_FSM_HEADER] = "HEADER",
    [HMR_FSM_NAME] = "NAME",     [HMR_FSM_CONTENT] = "CONTENT",
    [HMR_FSM_SYMBOL] = "SYMBOL", [HMR_FSM_ARROW] = "ARROW",
    [HMR_FSM_COMPO] = "COMPO",   [HMR_FSM_INSERT] = "INSERT",
    [HMR_FSM_MATCH] = "MATCH",   [HMR_FSM_TRANS] = "TRANS",
    [HMR_FSM_PAUSE] = "PAUSE",   [HMR_FSM_SLASHED] = "SLASHED",
    [HMR_FSM_END] = "END",       [HMR_FSM_ERROR] = "ERROR",
};

enum hmr_fsm_state fsm_next(enum hmr_fsm_state state, struct hmr_token *tok,
                            struct hmr_aux *aux, struct hmr_prof *prof)
{
    unsigned row = (unsigned)state;
    unsigned col = (unsigned)tok->id;
    struct trans const *const t = &transition[row][col];
    if (t->action(tok, state, aux, prof))
        return HMR_FSM_ERROR;
    return t->next;
}

char const *fsm_name(enum hmr_fsm_state state) { return state_name[state]; }

static enum hmr_rc nop(struct hmr_token *token, enum hmr_fsm_state state,
                       struct hmr_aux *aux, struct hmr_prof *prof)
{
    return HMR_SUCCESS;
}

static enum hmr_rc unexpected_eof(struct hmr_token *token,
                                  enum hmr_fsm_state state, struct hmr_aux *aux,
                                  struct hmr_prof *prof)
{
    error(token, "unexpected end-of-file");
    return HMR_PARSEERROR;
}

static enum hmr_rc header(struct hmr_token *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof)
{
    BUG(tok->id != HMR_TOKEN_WORD && tok->id != HMR_TOKEN_NEWLINE);
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
                                (unsigned long)(aux->prof.end - aux->prof.pos));
    }
    else
    {
        *(aux->prof.pos - 1) = '\0';
        aux_init(aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc field_name(struct hmr_token *tok, enum hmr_fsm_state state,
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
        aux->prof.begin = prof->buff;
        aux->prof.end = aux->prof.begin + HMR_BUFF_MAX;
    }
    aux->prof.pos = aux->prof.begin + 1;
    return HMR_SUCCESS;
}

static enum hmr_rc field_content(struct hmr_token *tok,
                                 enum hmr_fsm_state state, struct hmr_aux *aux,
                                 struct hmr_prof *prof)
{
    BUG(tok->id != HMR_TOKEN_WORD && tok->id != HMR_TOKEN_HMM &&
        tok->id != HMR_TOKEN_COMPO && tok->id != HMR_TOKEN_NEWLINE);

    if (tok->id == HMR_TOKEN_WORD || tok->id == HMR_TOKEN_HMM ||
        tok->id == HMR_TOKEN_COMPO)
    {
        if (aux->prof.pos > aux->prof.begin + 1)
        {
            *(aux->prof.pos - 1) = ' ';
            aux->prof.pos++;
        }
        aux->prof.pos = memccpy(aux->prof.pos - 1, tok->value, '\0',
                                (unsigned long)(aux->prof.end - aux->prof.pos));
    }
    else
    {
        if (aux->prof.pos == aux->prof.begin + 1)
        {
            error(tok, "expected content before end-of-line");
            return HMR_PARSEERROR;
        }
        *(aux->prof.pos - 1) = '\0';
        aux_init(aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc hmm(struct hmr_token *tok, enum hmr_fsm_state state,
                       struct hmr_aux *aux, struct hmr_prof *prof)
{
    aux->prof.begin = prof->symbols;
    aux->prof.end = aux->prof.begin + HMR_SYMBOLS_MAX;
    aux->prof.pos = aux->prof.begin + 1;
    return check_required_metadata(prof);
}

static enum hmr_rc symbol(struct hmr_token *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof)
{
    BUG(tok->id != HMR_TOKEN_WORD && tok->id != HMR_TOKEN_NEWLINE);
    if (tok->id == HMR_TOKEN_WORD)
    {
        *(aux->prof.pos - 1) = *tok->value;
        aux->prof.pos++;
    }
    else
    {
        *(aux->prof.pos - 1) = '\0';
        prof->symbols_size = (unsigned)strlen(prof->symbols);
        prof->node.symbols_size = prof->symbols_size;
        aux_init(aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc compo(struct hmr_token *tok, enum hmr_fsm_state state,
                         struct hmr_aux *aux, struct hmr_prof *prof)
{
    BUG(tok->id != HMR_TOKEN_WORD && tok->id != HMR_TOKEN_NEWLINE);
    if (tok->id == HMR_TOKEN_WORD)
    {
        if (aux->node.idx >= prof->symbols_size)
        {
            error(tok, "too many compo numbers");
            return HMR_PARSEERROR;
        }
        if (to_double(tok->value, prof->node.compo + aux->node.idx++))
        {
            error(tok, DEC_ERROR);
            return HMR_PARSEERROR;
        }
    }
    else
    {
        if (aux->node.idx != prof->symbols_size)
        {
            error(tok, "compo length not equal to symbols length");
            return HMR_PARSEERROR;
        }
        aux_init(aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc insert(struct hmr_token *tok, enum hmr_fsm_state state,
                          struct hmr_aux *aux, struct hmr_prof *prof)
{
    BUG(tok->id != HMR_TOKEN_WORD && tok->id != HMR_TOKEN_NEWLINE);
    if (tok->id == HMR_TOKEN_WORD)
    {
        if (aux->node.idx >= prof->symbols_size)
        {
            error(tok, "too many insert numbers");
            return HMR_PARSEERROR;
        }
        if (to_double(tok->value, prof->node.insert + aux->node.idx++))
        {
            error(tok, DEC_ERROR);
            return HMR_PARSEERROR;
        }
    }
    else
    {
        if (aux->node.idx != prof->symbols_size)
        {
            error(tok, "insert length not equal to symbols length");
            return HMR_PARSEERROR;
        }
        aux_init(aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc match(struct hmr_token *tok, enum hmr_fsm_state state,
                         struct hmr_aux *aux, struct hmr_prof *prof)
{
    BUG(tok->id != HMR_TOKEN_WORD && tok->id != HMR_TOKEN_NEWLINE);
    if (tok->id == HMR_TOKEN_WORD)
    {
        if (state == HMR_FSM_PAUSE)
        {
            unsigned i = (unsigned)strtoul(tok->value, NULL, 10);

            if (i == 0)
            {
                error(tok, "failed to convert integer");
                return HMR_PARSEERROR;
            }
            prof->node.idx = i;
            return HMR_SUCCESS;
        }
        if (aux->node.idx >= prof->symbols_size)
        {
            if (aux->node.idx >= prof->symbols_size + HMR_MATCH_EXCESS_SIZE)
            {
                error(tok, "too many match numbers");
                return HMR_PARSEERROR;
            }
            aux->node.idx++;
            return HMR_SUCCESS;
        }
        if (to_double(tok->value, prof->node.match + aux->node.idx++))
        {
            error(tok, DEC_ERROR);
            return HMR_PARSEERROR;
        }
    }
    else
    {
        if (aux->node.idx > prof->symbols_size + HMR_MATCH_EXCESS_SIZE)
        {
            error(tok, "too many match numbers");
            return HMR_PARSEERROR;
        }
        aux_init(aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc trans(struct hmr_token *tok, enum hmr_fsm_state state,
                         struct hmr_aux *aux, struct hmr_prof *prof)
{
    BUG(tok->id != HMR_TOKEN_WORD && tok->id != HMR_TOKEN_NEWLINE);
    if (tok->id == HMR_TOKEN_WORD)
    {
        if (aux->node.idx >= HMR_TRANS_SIZE)
        {
            error(tok, "too many trans numbers");
            return HMR_PARSEERROR;
        }
        if (to_double(tok->value, prof->node.trans + aux->node.idx++))
        {
            error(tok, DEC_ERROR);
            return HMR_PARSEERROR;
        }
    }
    else
    {
        if (aux->node.idx != HMR_TRANS_SIZE)
        {
            error(tok, "trans length not equal to " XSTR(HMR_TRANS_SIZE));
            return HMR_PARSEERROR;
        }
        aux_init(aux);
    }
    return HMR_SUCCESS;
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

static enum hmr_rc check_required_metadata(struct hmr_prof *prof)
{
    if (prof->meta.acc[0] == '\0')
    {
        error(prof, "missing ACC field");
        return HMR_PARSEERROR;
    }
    if (prof->meta.desc[0] == '\0')
    {
        error(prof, "missing DESC field");
        return HMR_PARSEERROR;
    }
    if (prof->meta.leng[0] == '\0')
    {
        error(prof, "missing LENG field");
        return HMR_PARSEERROR;
    }
    if (prof->meta.alph[0] == '\0')
    {
        error(prof, "missing ALPH field");
        return HMR_PARSEERROR;
    }
    return HMR_SUCCESS;
}
