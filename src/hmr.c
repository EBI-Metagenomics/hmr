#include "hmr/hmr.h"
#include "fsm.h"
#include "prof.h"
#include "token.h"
#include <stdlib.h>

struct hmr
{
    FILE *restrict fd;
    enum fsm_state state;
    struct token tok;
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

enum hmr_rc hmr_read(struct hmr *hmr, struct hmr_prof *prof)
{
    return prof_next(prof, hmr->fd, &hmr->state, &hmr->tok);
}

void hmr_close(struct hmr *hmr) { hmr->fd = NULL; }

void hmr_del(struct hmr const *hmr)
{
    if (hmr)
    {
        free((void *)hmr);
    }
}
