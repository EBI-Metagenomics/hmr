#include "hmr/prof.h"
#include "fsm.h"
#include "prof.h"
#include "token.h"

static void prof_init(struct hmr_prof *prof);

enum hmr_rc hmr_prof_read(struct hmr_prof *prof) { return HMR_SUCCESS; }

enum hmr_rc prof_next(struct hmr_prof *prof, FILE *restrict fd,
                      enum fsm_state *state, struct token *tok)
{
    prof_init(prof);
    while (token_next(fd, tok))
    {
        *state = fsm_next(*state, tok, prof);
    }
    return HMR_SUCCESS;
}

void prof_reset_tmp(struct hmr_prof *prof)
{
    prof->__.begin = NULL;
    prof->__.pos = NULL;
    prof->__.end = NULL;
}

static void prof_init(struct hmr_prof *prof)
{
    prof->header[0] = '\0';
    prof->meta.NAME[0] = '\0';
    prof->meta.ACC[0] = '\0';
    prof->meta.DESC[0] = '\0';
    prof->meta.LENG[0] = '\0';
    prof->meta.ALPH[0] = '\0';
    prof->symbols_size = 0;
    prof->symbols[0] = '\0';
    prof_reset_tmp(prof);
}
