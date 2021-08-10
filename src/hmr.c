#include "hmr/hmr.h"
#include "fsm.h"
#include "hmr.h"
#include "prof.h"
#include "token.h"
#include <stdlib.h>

struct hmr
{
    FILE *restrict fd;
    enum fsm_state state;
    struct token tok;

    struct hmr_aux aux;
};

struct hmr *hmr_new(void)
{
    struct hmr *hmr = malloc(sizeof(*hmr));
    if (!hmr)
        return NULL;

    hmr->fd = NULL;
    fsm_init(&hmr->state);
    token_init(&hmr->tok);
    return hmr;
}

enum hmr_rc hmr_open(struct hmr *hmr, FILE *restrict fd)
{
    hmr->fd = fd;
    return HMR_SUCCESS;
}

enum hmr_rc hmr_next_prof(struct hmr *hmr, struct hmr_prof *prof)
{
    return prof_next_prof(prof, hmr->fd, &hmr->aux, &hmr->state, &hmr->tok);
}

enum hmr_rc hmr_next_node(struct hmr *hmr, struct hmr_prof *prof)
{
    return prof_next_node(prof, hmr->fd, &hmr->aux, &hmr->state, &hmr->tok);
}

void hmr_close(struct hmr *hmr) { hmr->fd = NULL; }

void hmr_del(struct hmr const *hmr)
{
    if (hmr)
    {
        free((void *)hmr);
    }
}

void hmr_aux_reset(struct hmr_aux *aux)
{
    aux->prof.begin = NULL;
    aux->prof.pos = NULL;
    aux->prof.end = NULL;
    aux->node.idx = 0;
    aux->node.begin = NULL;
    aux->node.pos = NULL;
    aux->node.end = NULL;
}
