#ifndef ERROR_H
#define ERROR_H

#include "hmr/rc.h"

struct hmr_tok;

enum hmr_rc error(enum hmr_rc rc, char *dst, char const *msg);
enum hmr_rc error_io(char *dst, int errnum);
enum hmr_rc error_parse(struct hmr_tok *tok, char const *msg);

#endif
