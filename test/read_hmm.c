#include "hmr/hmr.h"
#include "hope/hope.h"

void test_hmm(void);

int main(void)
{
    test_hmm();
    return hope_status();
}

void test_hmm(void)
{
    FILE *fd = fopen("/Users/horta/code/hmmer-reader/data/PF02545.hmm", "r");

    struct hmr *hmr = hmr_new();

    hmr_open(hmr, fd);

    HMR_PROF_DECLARE(prof);

    while (hmr_next_prof(hmr, &prof))
    {
        while (hmr_next_node(hmr, &prof))
        {
        }
    }
    /* hmr_prof_dump(&prof, stdout); */
    /* hmr_node_dump(&prof.node, stdout); */

    hmr_close(hmr);

    fclose(fd);
}
