#include "hmr/hmr.h"
#include "fsm.h"
#include "prof.h"
#include "tok.h"

void hmr_init(struct hmr *hmr)
{
    hmr->fd = NULL;
    fsm_init(&hmr->state);
    hmr->error[0] = '\0';
    tok_init(&hmr->tok, hmr->error);
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

void hmr_clear_error(struct hmr *hmr) { hmr->error[0] = '\0'; }

void hmr_close(struct hmr *hmr)
{
    hmr->fd = NULL;
    hmr->error[0] = '\0';
}
