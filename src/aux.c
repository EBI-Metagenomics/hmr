#include "aux.h"
#include "hmr/aux.h"
#include <stdlib.h>

void aux_init(struct hmr_aux *aux)
{
    aux->prof.begin = NULL;
    aux->prof.pos = NULL;
    aux->prof.end = NULL;
    aux->node.idx = 0;
    aux->node.begin = NULL;
    aux->node.pos = NULL;
    aux->node.end = NULL;
}
