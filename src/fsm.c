#include "fsm.h"
#include "aux.h"
#include "bug.h"
#include "error.h"
#include "hmr/aux.h"
#include "hmr/prof.h"
#include "hmr/tok.h"
#include "tok.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define XSTR(s) STR(s)
#define STR(s) #s

#define DEC_ERROR "failed to parse decimal number"

struct args
{
    struct hmr_tok *tok;
    enum hmr_fsm_state state;
    struct hmr_aux *aux;
    struct hmr_prof *prof;
};

struct trans
{
    enum hmr_fsm_state const next;
    enum hmr_rc (*action)(struct args *a);
};

static enum hmr_rc nop(struct args *a) { return HMR_SUCCESS; }

static enum hmr_rc unexpect_eof(struct args *a)
{
    return error_parse(a->tok, "unexpected end-of-file");
}

static enum hmr_rc unexpect_eon(struct args *a)
{
    return error_parse(a->tok, "unexpected end-of-node");
}

static enum hmr_rc unexpect_symbol(struct args *a)
{
    return error_parse(a->tok, "unexpected symbol");
}

static enum hmr_rc unexpect_token(struct args *a)
{
    return error_parse(a->tok, "unexpected token");
}

static enum hmr_rc unexpect_newline(struct args *a)
{
    return error_parse(a->tok, "unexpected newline");
}

static enum hmr_rc header(struct args *a);

static enum hmr_rc field_name(struct args *a);

static enum hmr_rc field_content(struct args *a);

static enum hmr_rc hmm(struct args *a);

static enum hmr_rc symbol(struct args *a);

static enum hmr_rc compo(struct args *a);

static enum hmr_rc insert(struct args *a);

static enum hmr_rc match(struct args *a);

static enum hmr_rc trans(struct args *a);

static enum hmr_rc to_double(char const *str, double *val);

static enum hmr_rc check_header(struct hmr_prof *prof);

static enum hmr_rc check_required_metadata(struct hmr_prof *prof);

static struct trans const transition[][6] = {
    [HMR_FSM_BEGIN] = {[HMR_TOK_WORD] = {HMR_FSM_HEADER, &header},
                       [HMR_TOK_NEWLINE] = {HMR_FSM_BEGIN, &nop},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &header},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &header},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &header},
                       [HMR_TOK_EOF] = {HMR_FSM_END, &nop}},
    [HMR_FSM_HEADER] = {[HMR_TOK_WORD] = {HMR_FSM_HEADER, &header},
                        [HMR_TOK_NEWLINE] = {HMR_FSM_NAME, &header},
                        [HMR_TOK_HMM] = {HMR_FSM_ERROR, &header},
                        [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &header},
                        [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &header},
                        [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_NAME] = {[HMR_TOK_WORD] = {HMR_FSM_CONTENT, &field_name},
                      [HMR_TOK_NEWLINE] = {HMR_FSM_ERROR, &unexpect_newline},
                      [HMR_TOK_HMM] = {HMR_FSM_SYMBOL, &hmm},
                      [HMR_TOK_COMPO] = {HMR_FSM_PAUSE, &nop},
                      [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_token},
                      [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_CONTENT] = {[HMR_TOK_WORD] = {HMR_FSM_CONTENT, &field_content},
                         [HMR_TOK_NEWLINE] = {HMR_FSM_NAME, &field_content},
                         [HMR_TOK_HMM] = {HMR_FSM_CONTENT, &field_content},
                         [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_token},
                         [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_token},
                         [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_SYMBOL] = {[HMR_TOK_WORD] = {HMR_FSM_SYMBOL, &symbol},
                        [HMR_TOK_NEWLINE] = {HMR_FSM_ARROW, &symbol},
                        [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_symbol},
                        [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_symbol},
                        [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_symbol},
                        [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_ARROW] = {[HMR_TOK_WORD] = {HMR_FSM_ARROW, &nop},
                       [HMR_TOK_NEWLINE] = {HMR_FSM_PAUSE, &nop},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_token},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_token},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_token},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_COMPO] = {[HMR_TOK_WORD] = {HMR_FSM_COMPO, &compo},
                       [HMR_TOK_NEWLINE] = {HMR_FSM_INSERT, &compo},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_INSERT] = {[HMR_TOK_WORD] = {HMR_FSM_INSERT, &insert},
                        [HMR_TOK_NEWLINE] = {HMR_FSM_TRANS, &insert},
                        [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_eon},
                        [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_eon},
                        [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_eon},
                        [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_MATCH] = {[HMR_TOK_WORD] = {HMR_FSM_MATCH, &match},
                       [HMR_TOK_NEWLINE] = {HMR_FSM_INSERT, &match},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_TRANS] = {[HMR_TOK_WORD] = {HMR_FSM_TRANS, &trans},
                       [HMR_TOK_NEWLINE] = {HMR_FSM_PAUSE, &trans},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_PAUSE] = {[HMR_TOK_WORD] = {HMR_FSM_MATCH, &match},
                       [HMR_TOK_NEWLINE] = {HMR_FSM_ERROR, &unexpect_newline},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_token},
                       [HMR_TOK_COMPO] = {HMR_FSM_COMPO, &nop},
                       [HMR_TOK_SLASH] = {HMR_FSM_SLASHED, &nop},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_SLASHED] = {[HMR_TOK_WORD] = {HMR_FSM_ERROR, &unexpect_token},
                         [HMR_TOK_NEWLINE] = {HMR_FSM_BEGIN, &nop},
                         [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_token},
                         [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_token},
                         [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_token},
                         [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
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

enum hmr_fsm_state fsm_next(enum hmr_fsm_state state, struct hmr_tok *tok,
                            struct hmr_aux *aux, struct hmr_prof *prof)
{
    unsigned row = (unsigned)state;
    unsigned col = (unsigned)tok->id;
    struct trans const *const t = &transition[row][col];
    struct args args = {tok, state, aux, prof};
    if (t->action(&args))
        return HMR_FSM_ERROR;
    return t->next;
}

char const *fsm_name(enum hmr_fsm_state state) { return state_name[state]; }

static enum hmr_rc header(struct args *a)
{
    BUG(a->tok->id != HMR_TOK_WORD && a->tok->id != HMR_TOK_NEWLINE);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->aux->prof.pos > a->aux->prof.begin + 1)
        {
            *(a->aux->prof.pos - 1) = ' ';
            a->aux->prof.pos++;
        }
        else
        {
            a->aux->prof.begin = a->prof->header;
            a->aux->prof.pos = a->aux->prof.begin + 1;
            a->aux->prof.end = a->aux->prof.begin + HMR_HEADER_MAX;
        }
        a->aux->prof.pos =
            memccpy(a->aux->prof.pos - 1, a->tok->value, '\0',
                    (unsigned long)(a->aux->prof.end - a->aux->prof.pos));
    }
    else
    {
        *(a->aux->prof.pos - 1) = '\0';
        if (check_header(a->prof))
            return error_parse(a->tok, "invalid header");
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc field_name(struct args *a)
{
    if (!strcmp(a->tok->value, "NAME"))
    {
        a->aux->prof.begin = a->prof->meta.name;
        a->aux->prof.end = a->aux->prof.begin + HMR_NAME_MAX;
    }
    else if (!strcmp(a->tok->value, "ACC"))
    {
        a->aux->prof.begin = a->prof->meta.acc;
        a->aux->prof.end = a->aux->prof.begin + HMR_ACC_MAX;
    }
    else if (!strcmp(a->tok->value, "DESC"))
    {
        a->aux->prof.begin = a->prof->meta.desc;
        a->aux->prof.end = a->aux->prof.begin + HMR_DESC_MAX;
    }
    else if (!strcmp(a->tok->value, "LENG"))
    {
        a->aux->prof.begin = a->prof->meta.leng;
        a->aux->prof.end = a->aux->prof.begin + HMR_LENG_MAX;
    }
    else if (!strcmp(a->tok->value, "ALPH"))
    {
        a->aux->prof.begin = a->prof->meta.alph;
        a->aux->prof.end = a->aux->prof.begin + HMR_ALPH_MAX;
    }
    else
    {
        a->aux->prof.begin = a->prof->buff;
        a->aux->prof.end = a->aux->prof.begin + HMR_BUFF_MAX;
    }
    a->aux->prof.pos = a->aux->prof.begin + 1;
    return HMR_SUCCESS;
}

static enum hmr_rc field_content(struct args *a)
{
    BUG(a->tok->id != HMR_TOK_WORD && a->tok->id != HMR_TOK_HMM &&
        a->tok->id != HMR_TOK_COMPO && a->tok->id != HMR_TOK_NEWLINE);

    if (a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_HMM ||
        a->tok->id == HMR_TOK_COMPO)
    {
        if (a->aux->prof.pos > a->aux->prof.begin + 1)
        {
            *(a->aux->prof.pos - 1) = ' ';
            a->aux->prof.pos++;
        }
        a->aux->prof.pos =
            memccpy(a->aux->prof.pos - 1, a->tok->value, '\0',
                    (unsigned long)(a->aux->prof.end - a->aux->prof.pos));
    }
    else
    {
        if (a->aux->prof.pos == a->aux->prof.begin + 1)
            return error_parse(a->tok, "expected content before end-of-line");
        *(a->aux->prof.pos - 1) = '\0';
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc hmm(struct args *a)
{
    a->aux->prof.begin = a->prof->symbols;
    a->aux->prof.end = a->aux->prof.begin + HMR_SYMBOLS_MAX;
    a->aux->prof.pos = a->aux->prof.begin + 1;
    return check_required_metadata(a->prof);
}

static enum hmr_rc symbol(struct args *a)
{
    BUG(a->tok->id != HMR_TOK_WORD && a->tok->id != HMR_TOK_NEWLINE);
    if (a->tok->id == HMR_TOK_WORD)
    {
        *(a->aux->prof.pos - 1) = *a->tok->value;
        a->aux->prof.pos++;
    }
    else
    {
        *(a->aux->prof.pos - 1) = '\0';
        a->prof->symbols_size = (unsigned)strlen(a->prof->symbols);
        a->prof->node.symbols_size = a->prof->symbols_size;
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc compo(struct args *a)
{
    BUG(a->tok->id != HMR_TOK_WORD && a->tok->id != HMR_TOK_NEWLINE);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->aux->node.idx >= a->prof->symbols_size)
            return error_parse(a->tok, "too many compo numbers");

        if (to_double(a->tok->value, a->prof->node.compo + a->aux->node.idx++))
            return error_parse(a->tok, DEC_ERROR);
    }
    else
    {
        if (a->aux->node.idx != a->prof->symbols_size)
            return error_parse(a->tok,
                               "compo length not equal to symbols length");
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc insert(struct args *a)
{
    BUG(a->tok->id != HMR_TOK_WORD && a->tok->id != HMR_TOK_NEWLINE);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->aux->node.idx >= a->prof->symbols_size)
            return error_parse(a->tok, "too many insert numbers");

        if (to_double(a->tok->value, a->prof->node.insert + a->aux->node.idx++))
            return error_parse(a->tok, DEC_ERROR);
    }
    else
    {
        if (a->aux->node.idx != a->prof->symbols_size)
            return error_parse(a->tok,
                               "insert length not equal to symbols length");
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc match(struct args *a)
{
    BUG(a->tok->id != HMR_TOK_WORD && a->tok->id != HMR_TOK_NEWLINE);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->state == HMR_FSM_PAUSE)
        {
            unsigned i = (unsigned)strtoul(a->tok->value, NULL, 10);
            if (i == 0)
                return error_parse(a->tok, "failed to convert integer");

            a->prof->node.idx = i;
            return HMR_SUCCESS;
        }
        if (a->aux->node.idx >= a->prof->symbols_size)
        {
            if (a->aux->node.idx >=
                a->prof->symbols_size + HMR_MATCH_EXCESS_SIZE)
                return error_parse(a->tok, "too many match numbers");

            a->aux->node.idx++;
            return HMR_SUCCESS;
        }
        if (to_double(a->tok->value, a->prof->node.match + a->aux->node.idx++))
            return error_parse(a->tok, DEC_ERROR);
    }
    else
    {
        if (a->aux->node.idx > a->prof->symbols_size + HMR_MATCH_EXCESS_SIZE)
            return error_parse(a->tok, "too many match numbers");
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc trans(struct args *a)
{
    BUG(a->tok->id != HMR_TOK_WORD && a->tok->id != HMR_TOK_NEWLINE);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->aux->node.idx >= HMR_TRANS_SIZE)
            return error_parse(a->tok, "too many trans numbers");

        if (to_double(a->tok->value, a->prof->node.trans + a->aux->node.idx++))
            return error_parse(a->tok, DEC_ERROR);
    }
    else
    {
        if (a->aux->node.idx != HMR_TRANS_SIZE)
            return error_parse(
                a->tok, "trans length not equal to " XSTR(HMR_TRANS_SIZE));
        aux_init(a->aux);
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

    if (strchr(str, '\0') != ptr)
        return HMR_PARSEERROR;

    return HMR_SUCCESS;
}

#define HEADER_EXAMPLE "HMMER3/f [3.3.1 | Jul 2020]"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static enum hmr_rc check_header(struct hmr_prof *prof)
{
    char tmp[sizeof HEADER_EXAMPLE + 10];
    if (strlen(prof->header) >= ARRAY_SIZE(tmp))
        return HMR_PARSEERROR;

    strcpy(tmp, prof->header);
    char *ptr = NULL;
    char *tok = NULL;

    if (!(tok = strtok_r(tmp, " ", &ptr)))
        return HMR_PARSEERROR;

    if (strcmp(tok, "HMMER3/f"))
        return HMR_PARSEERROR;

    if (!(tok = strtok_r(NULL, " ", &ptr)))
        return HMR_PARSEERROR;

    if (*tok != '[')
        return HMR_PARSEERROR;

    if (!(tok = strtok_r(NULL, " ", &ptr)))
        return HMR_PARSEERROR;

    if (*tok != '|')
        return HMR_PARSEERROR;

    /* Month */
    if (!(tok = strtok_r(NULL, " ", &ptr)))
        return HMR_PARSEERROR;

    /* Year] */
    if (!(tok = strtok_r(NULL, " ", &ptr)))
        return HMR_PARSEERROR;

    if (!(tok = strchr(tok, ']')))
        return HMR_PARSEERROR;

    if (strtok_r(NULL, " ", &ptr))
        return HMR_PARSEERROR;

    return HMR_SUCCESS;
}

static enum hmr_rc check_required_metadata(struct hmr_prof *prof)
{
    if (prof->meta.acc[0] == '\0')
        return error_parse(prof, "missing ACC field");

    if (prof->meta.desc[0] == '\0')
        return error_parse(prof, "missing DESC field");

    if (prof->meta.leng[0] == '\0')
        return error_parse(prof, "missing LENG field");

    if (prof->meta.alph[0] == '\0')
        return error_parse(prof, "missing ALPH field");

    return HMR_SUCCESS;
}
