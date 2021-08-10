#include "node.h"
#include "hmr/node.h"
#include <stdlib.h>

void node_reset_tmp(struct hmr_node *node)
{
    node->__.idx = 0;
    node->__.begin = NULL;
    node->__.pos = NULL;
    node->__.end = NULL;
}
