#include "node.h"
#include "hmr/node.h"
#include <math.h>
#include <stdlib.h>

void hmr_node_dump(struct hmr_node const *node, FILE *fd)
{
    if (node->idx == 0)
        fprintf(fd, "COMPO");
    else
        fprintf(fd, "%*u", 5, node->idx);
    for (unsigned i = 0; i < node->symbols_size; ++i)
        fprintf(fd, " %.5f", node->compo[i]);
    fprintf(fd, "\n");

    fprintf(fd, "     ");
    for (unsigned i = 0; i < node->symbols_size; ++i)
        fprintf(fd, " %.5f", node->insert[i]);
    fprintf(fd, "\n");

    fprintf(fd, "     ");
    for (unsigned i = 0; i < HMR_TRANS_SIZE; ++i)
        fprintf(fd, " %.5f", node->trans[i]);
    fprintf(fd, "\n");
}

void node_init(struct hmr_node *node)
{
    node->symbols_size = 0;
    node->idx = 0;
    for (unsigned i = 0; i < HMR_SYMBOLS_MAX; ++i)
    {
        node->compo[i] = NAN;
        node->insert[i] = NAN;
    }
    for (unsigned i = 0; i < HMR_TRANS_SIZE; ++i)
        node->trans[i] = NAN;
}
