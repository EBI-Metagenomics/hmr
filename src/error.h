#ifndef ERROR_H
#define ERROR_H

#include "hmr/hmr.h"

struct hmr_prof;
struct hmr_token;

void error_chr(char *dst, char const *msg);
void error_prof(struct hmr_prof *prof, char const *msg);
void error_tok(struct hmr_token *tok, char const *msg);

#define error(x, msg)                                                          \
    _Generic((x), \
            struct hmr_token *                                           \
             : error_tok, struct hmr_prof *                                    \
             : error_prof, char * : error_chr)(x, msg)

#endif
