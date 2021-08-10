#include "hmr/prof.h"
#include "fsm.h"
#include "hmr.h"
#include "node.h"
#include "prof.h"
#include "token.h"

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
                           struct hmr_aux *aux, enum fsm_state *state,
                           struct token *tok)
{
    hmr_aux_reset(aux);
    /* printf(".. reenter\n"); */
    while (token_next(fd, tok))
    {
        /* if (tok->id == TOKEN_NEWLINE) */
        /*     printf("%s: _\n", fsm_name(*state)); */
        /* else */
        /*     printf("%s: %s\n", fsm_name(*state), tok->value); */
        *state = fsm_next(*state, tok, aux, prof);

        if (*state == FSM_PAUSE)
            break;

        if (*state == FSM_BEGIN)
        {
            if (tok->value)
                return HMR_ENDPROF;
            return HMR_FAILURE;
        }
    }
    if (*state == FSM_BEGIN)
    {
        if (tok->value)
            return HMR_FAILURE;
        return HMR_ENDFILE;
    }
    return HMR_SUCCESS;
}

enum hmr_rc prof_next_prof(struct hmr_prof *prof, FILE *restrict fd,
                           struct hmr_aux *aux, enum fsm_state *state,
                           struct token *tok)
{
    hmr_prof_init(prof);
    return prof_next_node(prof, fd, aux, state, tok);
}
