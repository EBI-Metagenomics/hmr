#ifndef ERROR_H
#define ERROR_H

#include "hmr/hmr.h"
#include "hmr/rc.h"

struct hmr_prof;
struct hmr_tok;

enum hmr_rc error_io(char *dst, int errnum);
enum hmr_rc error_runtime(char *dst, char const *fmt, ...);

#define error_parse(x, ...)                                                    \
    _Generic((x), struct hmr_tok *                                             \
             : __error_parse_tok, struct hmr_prof *                            \
             : __error_parse_prof)(x, __VA_ARGS__)

enum hmr_rc __error_parse_prof(struct hmr_prof *prof, char const *fmt, ...);
enum hmr_rc __error_parse_tok(struct hmr_tok *tok, char const *fmt, ...);

#endif
