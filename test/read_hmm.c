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

    struct hmr_prof prof;
    hmr_read(hmr, &prof);
    hmr_close(hmr);

    fclose(fd);
}
