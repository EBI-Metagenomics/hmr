#ifndef ERROR_H
#define ERROR_H

#include "hmr/hmr.h"
#include "hmr/rc.h"

struct hmr_prof;
struct hmr_tok;

enum hmr_rc error_io(char *dst, int errnum);
enum hmr_rc error_runtime(char *dst, char const func[], char const *msg);

#define error_parse(x, msg)                                                    \
    _Generic((x), struct hmr_tok *                                             \
             : __error_parse_tok, struct hmr_prof *                            \
             : __error_parse_prof)(x, msg)

enum hmr_rc __error_parse_prof(struct hmr_prof *prof, char const *msg);
enum hmr_rc __error_parse_tok(struct hmr_tok *tok, char const *msg);

#endif
