#include "fsm.h"
#include "aux.h"
#include "error.h"
#include "hmr/aux.h"
#include "hmr/prof.h"
#include "hmr/tok.h"
#include "hmr/trans.h"
#include "to.h"
#include "tok.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define XSTR(s) STR(s)
#define STR(s) #s

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

#define INT_ERROR "failed to parse integer"
#define DEC_ERROR "failed to parse decimal number"

struct args
{
    struct hmr_tok *tok;
    enum hmr_state state;
    struct hmr_aux *aux;
    struct hmr_prof *prof;
};

struct trans
{
    enum hmr_state const next;
    enum hmr_rc (*action)(struct args *a);
};

static char const arrows[HMR_TRANS_SIZE][5] = {
    [HMR_TRANS_MM] = "m->m", [HMR_TRANS_MI] = "m->i", [HMR_TRANS_MD] = "m->d",
    [HMR_TRANS_IM] = "i->m", [HMR_TRANS_II] = "i->i", [HMR_TRANS_DM] = "d->m",
    [HMR_TRANS_DD] = "d->d"};

static enum hmr_rc nop(struct args *a) { return HMR_SUCCESS; }

static enum hmr_rc arrow(struct args *a);

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

static enum hmr_rc unexpect_tok(struct args *a)
{
    return error_parse(a->tok, "unexpected token");
}

static enum hmr_rc unexpect_nl(struct args *a)
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

static enum hmr_rc check_header(struct hmr_prof *prof);

static enum hmr_rc check_required_metadata(struct hmr_prof *prof);

static struct trans const transition[][6] = {
    [HMR_FSM_BEGIN] = {[HMR_TOK_WORD] = {HMR_FSM_HEADER, &header},
                       [HMR_TOK_NL] = {HMR_FSM_BEGIN, &nop},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &header},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &header},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &header},
                       [HMR_TOK_EOF] = {HMR_FSM_END, &nop}},
    [HMR_FSM_HEADER] = {[HMR_TOK_WORD] = {HMR_FSM_HEADER, &header},
                        [HMR_TOK_NL] = {HMR_FSM_NAME, &header},
                        [HMR_TOK_HMM] = {HMR_FSM_ERROR, &header},
                        [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &header},
                        [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &header},
                        [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_NAME] = {[HMR_TOK_WORD] = {HMR_FSM_CONTENT, &field_name},
                      [HMR_TOK_NL] = {HMR_FSM_ERROR, &unexpect_nl},
                      [HMR_TOK_HMM] = {HMR_FSM_SYMBOL, &hmm},
                      [HMR_TOK_COMPO] = {HMR_FSM_PAUSE, &nop},
                      [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_tok},
                      [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_CONTENT] = {[HMR_TOK_WORD] = {HMR_FSM_CONTENT, &field_content},
                         [HMR_TOK_NL] = {HMR_FSM_NAME, &field_content},
                         [HMR_TOK_HMM] = {HMR_FSM_CONTENT, &field_content},
                         [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_tok},
                         [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_tok},
                         [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_SYMBOL] = {[HMR_TOK_WORD] = {HMR_FSM_SYMBOL, &symbol},
                        [HMR_TOK_NL] = {HMR_FSM_ARROW, &symbol},
                        [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_symbol},
                        [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_symbol},
                        [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_symbol},
                        [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_ARROW] = {[HMR_TOK_WORD] = {HMR_FSM_ARROW, &arrow},
                       [HMR_TOK_NL] = {HMR_FSM_PAUSE, &arrow},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_tok},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_tok},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_tok},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_COMPO] = {[HMR_TOK_WORD] = {HMR_FSM_COMPO, &compo},
                       [HMR_TOK_NL] = {HMR_FSM_INSERT, &compo},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_INSERT] = {[HMR_TOK_WORD] = {HMR_FSM_INSERT, &insert},
                        [HMR_TOK_NL] = {HMR_FSM_TRANS, &insert},
                        [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_eon},
                        [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_eon},
                        [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_eon},
                        [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_MATCH] = {[HMR_TOK_WORD] = {HMR_FSM_MATCH, &match},
                       [HMR_TOK_NL] = {HMR_FSM_INSERT, &match},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_TRANS] = {[HMR_TOK_WORD] = {HMR_FSM_TRANS, &trans},
                       [HMR_TOK_NL] = {HMR_FSM_PAUSE, &trans},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_eon},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_PAUSE] = {[HMR_TOK_WORD] = {HMR_FSM_MATCH, &match},
                       [HMR_TOK_NL] = {HMR_FSM_ERROR, &unexpect_nl},
                       [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_tok},
                       [HMR_TOK_COMPO] = {HMR_FSM_COMPO, &nop},
                       [HMR_TOK_SLASH] = {HMR_FSM_SLASHED, &nop},
                       [HMR_TOK_EOF] = {HMR_FSM_ERROR, &unexpect_eof}},
    [HMR_FSM_SLASHED] = {[HMR_TOK_WORD] = {HMR_FSM_ERROR, &unexpect_tok},
                         [HMR_TOK_NL] = {HMR_FSM_BEGIN, &nop},
                         [HMR_TOK_HMM] = {HMR_FSM_ERROR, &unexpect_tok},
                         [HMR_TOK_COMPO] = {HMR_FSM_ERROR, &unexpect_tok},
                         [HMR_TOK_SLASH] = {HMR_FSM_ERROR, &unexpect_tok},
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

enum hmr_state fsm_next(enum hmr_state state, struct hmr_tok *tok,
                        struct hmr_aux *aux, struct hmr_prof *prof)
{
    unsigned row = (unsigned)state;
    unsigned col = (unsigned)tok->id;
    struct trans const *const t = &transition[row][col];
    struct args args = {tok, state, aux, prof};
    if (t->action(&args)) return HMR_FSM_ERROR;
    return t->next;
}

char const *fsm_name(enum hmr_state state) { return state_name[state]; }

static enum hmr_rc arrow(struct args *a)
{
    assert(a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_NL);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->aux->idx >= HMR_TRANS_SIZE) return unexpect_tok(a);

        if (strcmp(a->tok->value, arrows[a->aux->idx]))
            return error_parse(a->tok, "expected %s", arrows[a->aux->idx]);
        a->aux->idx++;
    }
    else
    {
        if (a->aux->idx != HMR_TRANS_SIZE)
            return error_parse(a->tok, "unexpected end-of-line");
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc header(struct args *a)
{
    assert(a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_NL);
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
        if (check_header(a->prof)) return error_parse(a->tok, "invalid header");
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
    assert(a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_HMM ||
           a->tok->id == HMR_TOK_COMPO || a->tok->id == HMR_TOK_NL);

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
    assert(a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_NL);
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
    assert(a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_NL);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->aux->idx >= a->prof->symbols_size)
            return error_parse(a->tok, "too many compo numbers");

        if (!to_lprob(a->tok->value, a->prof->node.compo + a->aux->idx++))
            return error_parse(a->tok, DEC_ERROR);
    }
    else
    {
        if (a->aux->idx != a->prof->symbols_size)
            return error_parse(a->tok,
                               "compo length not equal to symbols length");
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc insert(struct args *a)
{
    assert(a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_NL);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->aux->idx >= a->prof->symbols_size)
            return error_parse(a->tok, "too many insert numbers");

        if (!to_lprob(a->tok->value, a->prof->node.insert + a->aux->idx++))
            return error_parse(a->tok, DEC_ERROR);
    }
    else
    {
        if (a->aux->idx != a->prof->symbols_size)
            return error_parse(a->tok,
                               "insert length not equal to symbols length");
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static bool read_map(struct args *a);

static enum hmr_rc read_match_excess(struct args *a)
{
    unsigned sz = a->prof->symbols_size;
    unsigned excess = MEMBER_SIZE(struct hmr_node, excess.buf) + 1;
    if (a->aux->idx >= sz + excess)
        return error_parse(a->tok, "too many match numbers");

    if (a->aux->idx == sz)
    {
        if (!read_map(a)) return error_parse(a->tok, INT_ERROR);
        return HMR_SUCCESS;
    }

    if (a->tok->value[0] == '\0' || a->tok->value[1] != '\0')
        return error_parse(a->tok, "excesses must be single character");

    a->prof->node.excess.buf[a->aux->idx++ - sz - 1] = a->tok->value[0];
    return HMR_SUCCESS;
}

static enum hmr_rc match(struct args *a)
{
    assert(a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_NL);
    unsigned sz = a->prof->symbols_size;
    unsigned excess = MEMBER_SIZE(struct hmr_node, excess.buf) + 1;
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->state == HMR_FSM_PAUSE)
        {
            if (!to_uint(a->tok->value, &a->prof->node.idx))
                return error_parse(a->tok, INT_ERROR);
            return HMR_SUCCESS;
        }
        if (a->aux->idx >= sz) return read_match_excess(a);

        if (!to_lprob(a->tok->value, a->prof->node.match + a->aux->idx++))
            return error_parse(a->tok, DEC_ERROR);
    }
    else
    {
        if (a->aux->idx > sz + excess)
            return error_parse(a->tok, "too many match numbers");
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

static enum hmr_rc trans(struct args *a)
{
    assert(a->tok->id == HMR_TOK_WORD || a->tok->id == HMR_TOK_NL);
    if (a->tok->id == HMR_TOK_WORD)
    {
        if (a->aux->idx >= HMR_TRANS_SIZE)
            return error_parse(a->tok, "too many trans numbers");

        if (!to_lprob(a->tok->value, a->prof->node.trans + a->aux->idx++))
            return error_parse(a->tok, DEC_ERROR);
    }
    else
    {
        if (a->aux->idx != HMR_TRANS_SIZE)
            return error_parse(
                a->tok, "trans length not equal to " XSTR(HMR_TRANS_SIZE));
        aux_init(a->aux);
    }
    return HMR_SUCCESS;
}

#define HEADER_EXAMPLE "HMMER3/f [3.3.1 | Jul 2020]"

static enum hmr_rc check_header(struct hmr_prof *prof)
{
    char tmp[sizeof HEADER_EXAMPLE + 10];
    if (strlen(prof->header) >= ARRAY_SIZE(tmp)) return HMR_PARSEERROR;

    strcpy(tmp, prof->header);
    char *ptr = NULL;
    char *tok = NULL;

    if (!(tok = strtok_r(tmp, " ", &ptr))) return HMR_PARSEERROR;

    if (strcmp(tok, "HMMER3/f")) return HMR_PARSEERROR;

    if (!(tok = strtok_r(NULL, " ", &ptr))) return HMR_PARSEERROR;

    if (*tok != '[') return HMR_PARSEERROR;

    if (!(tok = strtok_r(NULL, " ", &ptr))) return HMR_PARSEERROR;

    if (*tok != '|') return HMR_PARSEERROR;

    /* Month */
    if (!(tok = strtok_r(NULL, " ", &ptr))) return HMR_PARSEERROR;

    /* Year] */
    if (!(tok = strtok_r(NULL, " ", &ptr))) return HMR_PARSEERROR;

    if (!(tok = strchr(tok, ']'))) return HMR_PARSEERROR;

    if (strtok_r(NULL, " ", &ptr)) return HMR_PARSEERROR;

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

static bool read_map(struct args *a)
{
    if (a->tok->value[0] == '-' && a->tok->value[1] == '\0')
        a->prof->node.excess.map = HMR_NODE_MAP_NULL;
    else
    {
        if (!to_uint(a->tok->value, &a->prof->node.excess.map)) return false;
    }
    a->aux->idx++;
    return true;
}
