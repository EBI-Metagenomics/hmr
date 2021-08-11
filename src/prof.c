#include "hmr/prof.h"
#include "bug.h"
#include "fsm.h"
#include "hmr.h"
#include "hmr/token.h"
#include "node.h"
#include "prof.h"
#include "token.h"
#include <limits.h>
#include <stdlib.h>

void hmr_prof_dump(struct hmr_prof const *prof, FILE *fd)
{
    fprintf(fd, "HEADER: %s\n", prof->header);
    fprintf(fd, "  Name: %s\n", prof->meta.name);
    fprintf(fd, "   Acc: %s\n", prof->meta.acc);
    fprintf(fd, "  Desc: %s\n", prof->meta.desc);
    fprintf(fd, "  Leng: %s\n", prof->meta.leng);
    fprintf(fd, "  Alph: %s\n", prof->meta.alph);
    fprintf(fd, "  Name: %s\n", prof->meta.name);
    fprintf(fd, "  ");
    for (unsigned i = 0; i < prof->symbols_size; ++i)
    {
        fprintf(fd, "       %c", prof->symbols[i]);
    }
    fprintf(fd, "\n");
}

void hmr_prof_init(struct hmr_prof *prof)
{
    prof->header[0] = '\0';
    prof->meta.name[0] = '\0';
    prof->meta.acc[0] = '\0';
    prof->meta.desc[0] = '\0';
    prof->meta.leng[0] = '\0';
    prof->meta.alph[0] = '\0';
    prof->symbols_size = 0;
    prof->symbols[0] = '\0';
    node_init(&prof->node);
}

enum hmr_rc prof_next_node(struct hmr_prof *prof, FILE *restrict fd,
                           struct hmr_aux *aux, enum hmr_fsm_state *state,
                           struct hmr_token *tok)
{
    if (*state != HMR_FSM_PAUSE)
        return HMR_RUNTIMEERROR;

    aux_reset(aux);
    do
    {
        enum hmr_rc rc = HMR_SUCCESS;
        if ((rc = token_next(fd, tok)))
            return rc;

        *state = fsm_next(*state, tok, aux, prof);
        if (*state == HMR_FSM_ERROR)
            return HMR_PARSEERROR;
        if (*state == HMR_FSM_BEGIN)
            return HMR_ENDNODE;

    } while (*state != HMR_FSM_PAUSE);

    return HMR_SUCCESS;
}

enum hmr_rc prof_next_prof(struct hmr_prof *prof, FILE *restrict fd,
                           struct hmr_aux *aux, enum hmr_fsm_state *state,
                           struct hmr_token *tok)
{
    if (*state != HMR_FSM_BEGIN)
        return HMR_RUNTIMEERROR;

    hmr_prof_init(prof);
    aux_reset(aux);
    do
    {
        enum hmr_rc rc = HMR_SUCCESS;
        if ((rc = token_next(fd, tok)))
            return rc;

        if ((*state = fsm_next(*state, tok, aux, prof)) == HMR_FSM_ERROR)
            return HMR_PARSEERROR;

    } while (*state != HMR_FSM_PAUSE && *state != HMR_FSM_END);

    if (*state == HMR_FSM_END)
        return HMR_ENDFILE;

    return HMR_SUCCESS;
}

unsigned hmr_prof_length(struct hmr_prof const *prof)
{
    long v = strtol(prof->meta.leng, NULL, 10);
    if (v == LONG_MAX)
        return UINT_MAX;
    if (v == LONG_MIN)
        return 0;
    return (unsigned)v;
}

#if 0
static enum hmr_rc resume_prof(struct hmr_prof *prof, FILE *restrict fd,
                               struct hmr_aux *aux, enum hmr_fsm_state *state,
                               struct hmr_token *tok)
{
    enum hmr_rc rc = HMR_SUCCESS;

    BUG(*state != HMR_FSM_PAUSE);
    if (!(rc = token_next(fd, tok)))
        return rc;

    *state = fsm_next(*state, tok, aux, prof);
    if (*state != HMR_FSM_SLASHED)
        return HMR_PARSEERROR;

    if (!(rc = token_next(fd, tok)))
        return rc;

    *state = fsm_next(*state, tok, aux, prof);
    if (*state != HMR_FSM_BEGIN)
        return HMR_PARSEERROR;
    return HMR_SUCCESS;
}
#endif
