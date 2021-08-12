#ifndef HMR_AUX_H
#define HMR_AUX_H

#define HMR_ERROR_SIZE 128

struct hmr_aux
{
    union
    {
        struct
        {
            char *begin;
            char *pos;
            char *end;
        } prof;

        struct
        {
            unsigned idx;
            double *begin;
            double *pos;
            double *end;
        } node;
    };
    char error[HMR_ERROR_SIZE];
};

#endif
