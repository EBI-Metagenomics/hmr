#include "hmr/hmr.h"
#include "fsm.h"
#include "prof.h"
#include "tok.h"

void hmr_init(struct hmr *hmr, FILE *restrict fp)
{
    hmr->fp = fp;
    fsm_init(&hmr->state);
    hmr->error[0] = '\0';
    tok_init(&hmr->tok, hmr->error);
}

enum hmr_rc hmr_next_prof(struct hmr *hmr, struct hmr_prof *prof)
{
    return prof_next_prof(prof, hmr->fp, &hmr->aux, &hmr->state, &hmr->tok);
}

enum hmr_rc hmr_next_node(struct hmr *hmr, struct hmr_prof *prof)
{
    return prof_next_node(prof, hmr->fp, &hmr->aux, &hmr->state, &hmr->tok);
}

void hmr_clear_error(struct hmr *hmr) { hmr->error[0] = '\0'; }
