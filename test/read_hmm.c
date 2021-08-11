#include "hmr/hmr.h"
#include "hope/hope.h"

void test_hmm_3profs(void);
void test_hmm_empty(void);
void test_hmm_corrupted1(void);
void test_hmm_corrupted2(void);
void test_hmm_corrupted3(void);

void check_3profs0(struct hmr_prof *prof);
void check_3profs1(struct hmr_prof *prof);
void check_3profs2(struct hmr_prof *prof);

void (*check_prof[3])(struct hmr_prof *prof) = {
    check_3profs0,
    check_3profs1,
    check_3profs2,
};

int main(void)
{
    test_hmm_3profs();
    test_hmm_empty();
    test_hmm_corrupted1();
    test_hmm_corrupted2();
    test_hmm_corrupted3();
    return hope_status();
}

void test_hmm_3profs(void)
{
    FILE *fd = fopen(ASSETS "/three-profs.hmm", "r");
    NOTNULL(fd);
    unsigned symbol_size = 20;
    unsigned prof_size[] = {40, 235, 449};

    HMR_DECLARE(hmr);

    EQ(hmr_open(&hmr, fd), HMR_SUCCESS);

    HMR_PROF_DECLARE(prof);

    unsigned prof_idx = 0;
    enum hmr_rc rc = HMR_SUCCESS;
    while (!(rc = hmr_next_prof(&hmr, &prof)))
    {
        EQ(prof.symbols_size, symbol_size);
        unsigned node_idx = 0;
        while (!(rc = hmr_next_node(&hmr, &prof)))
        {
            EQ(prof.node.idx, node_idx);
            check_prof[prof_idx](&prof);
            node_idx++;
        }
        EQ(prof.node.idx, prof_size[prof_idx]);
        EQ(hmr_prof_length(&prof), prof_size[prof_idx]);
        prof_idx++;
    }
    EQ(prof_idx, 3);

    hmr_close(&hmr);

    fclose(fd);
}

void test_hmm_empty(void)
{
    FILE *fd = fopen(ASSETS "/empty.hmm", "r");
    NOTNULL(fd);

    HMR_DECLARE(hmr);

    EQ(hmr_open(&hmr, fd), HMR_SUCCESS);

    HMR_PROF_DECLARE(prof);

    EQ(hmr_next_prof(&hmr, &prof), HMR_ENDFILE);
    EQ(hmr_next_prof(&hmr, &prof), HMR_RUNTIMEERROR);

    hmr_close(&hmr);

    fclose(fd);
}

void test_hmm_corrupted1(void)
{
    FILE *fd = fopen("/Users/horta/data/corrupted1.hmm", "r");
    NOTNULL(fd);

    HMR_DECLARE(hmr);

    EQ(hmr_open(&hmr, fd), HMR_SUCCESS);

    HMR_PROF_DECLARE(prof);

    unsigned prof_idx = 0;
    enum hmr_rc rc = HMR_SUCCESS;
    while (!(rc = hmr_next_prof(&hmr, &prof)))
    {
        unsigned node_idx = 0;
        while (!(rc = hmr_next_node(&hmr, &prof)))
        {
            node_idx++;
        }
        if (prof_idx == 2 && node_idx == 5)
            EQ(rc, HMR_PARSEERROR);
        prof_idx++;
    }
    EQ(prof_idx, 3);
    EQ(rc, HMR_RUNTIMEERROR);

    hmr_close(&hmr);

    fclose(fd);
}

void test_hmm_corrupted2(void)
{
    FILE *fd = fopen("/Users/horta/data/corrupted2.hmm", "r");
    NOTNULL(fd);
    unsigned prof_size[] = {40};

    HMR_DECLARE(hmr);

    EQ(hmr_open(&hmr, fd), HMR_SUCCESS);

    HMR_PROF_DECLARE(prof);

    unsigned prof_idx = 0;
    enum hmr_rc rc = HMR_SUCCESS;
    while (!(rc = hmr_next_prof(&hmr, &prof)))
    {
        unsigned node_idx = 0;
        while (!(rc = hmr_next_node(&hmr, &prof)))
        {
            node_idx++;
        }
        EQ(prof.node.idx, prof_size[prof_idx]);
        EQ(hmr_prof_length(&prof), prof_size[prof_idx]);
        prof_idx++;
    }
    EQ(prof_idx, 1);
    EQ(rc, HMR_PARSEERROR);

    hmr_close(&hmr);

    fclose(fd);
}

void test_hmm_corrupted3(void)
{
    FILE *fd = fopen("/Users/horta/data/corrupted3.hmm", "r");
    NOTNULL(fd);
    unsigned prof_size[] = {40};

    HMR_DECLARE(hmr);

    EQ(hmr_open(&hmr, fd), HMR_SUCCESS);

    HMR_PROF_DECLARE(prof);

    unsigned prof_idx = 0;
    enum hmr_rc rc = HMR_SUCCESS;
    while (!(rc = hmr_next_prof(&hmr, &prof)))
    {
        unsigned node_idx = 0;
        while (!(rc = hmr_next_node(&hmr, &prof)))
        {
            node_idx++;
        }
        EQ(prof.node.idx, prof_size[prof_idx]);
        EQ(hmr_prof_length(&prof), 0);
        prof_idx++;
    }
    EQ(prof_idx, 0);
    EQ(rc, HMR_PARSEERROR);

    hmr_close(&hmr);

    fclose(fd);
}

void check_3profs0(struct hmr_prof *prof)
{
    if (prof->node.idx == 0)
    {
        CLOSE(prof->node.compo[0], 2.29746);
        CLOSE(prof->node.compo[prof->symbols_size - 1], 3.82158);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.00201);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 2], 0.0);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], NAN);
    }
    if (prof->node.idx == 1)
    {
        CLOSE(prof->node.match[0], 0.34643);
        CLOSE(prof->node.match[prof->symbols_size - 1], 7.58384);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.00201);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], 0.95510);
    }
    if (prof->node.idx == 40)
    {
        CLOSE(prof->node.match[0], 3.29199);
        CLOSE(prof->node.match[prof->symbols_size - 1], 3.78781);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.00135);
        CLOSE(prof->node.trans[2], NAN);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 2], 0.0);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], NAN);
    }
}

void check_3profs1(struct hmr_prof *prof)
{
    if (prof->node.idx == 0)
    {
        CLOSE(prof->node.compo[0], 2.47491);
        CLOSE(prof->node.compo[prof->symbols_size - 1], 3.46896);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.02633);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 2], 0.0);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], NAN);
    }
    if (prof->node.idx == 1)
    {
        CLOSE(prof->node.match[0], 3.26601);
        CLOSE(prof->node.match[prof->symbols_size - 1], 4.14252);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.02633);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], 0.95510);
    }
    if (prof->node.idx == 235)
    {
        CLOSE(prof->node.match[0], 2.75686);
        CLOSE(prof->node.match[prof->symbols_size - 1], 3.85418);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.01780);
        CLOSE(prof->node.trans[2], NAN);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 2], 0.0);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], NAN);
    }
}

void check_3profs2(struct hmr_prof *prof)
{
    if (prof->node.idx == 0)
    {
        CLOSE(prof->node.compo[0], 2.55148);
        CLOSE(prof->node.compo[prof->symbols_size - 1], 3.24305);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.01335);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 2], 0.0);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], NAN);
    }
    if (prof->node.idx == 1)
    {
        CLOSE(prof->node.match[0], 2.77993);
        CLOSE(prof->node.match[prof->symbols_size - 1], 2.88211);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.01335);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], 0.95510);
    }
    if (prof->node.idx == 449)
    {
        CLOSE(prof->node.match[0], 3.39753);
        CLOSE(prof->node.match[prof->symbols_size - 1], 4.58563);
        CLOSE(prof->node.insert[0], 2.68618);
        CLOSE(prof->node.insert[prof->symbols_size - 1], 3.61503);
        CLOSE(prof->node.trans[0], 0.00900);
        CLOSE(prof->node.trans[2], NAN);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 2], 0.0);
        CLOSE(prof->node.trans[HMR_TRANS_SIZE - 1], NAN);
    }
}
