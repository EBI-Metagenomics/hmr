#include "error.h"
#include "aux.h"
#include "hmr/aux.h"
#include "hmr/error.h"
#include "hmr/prof.h"
#include "hmr/tok.h"
#include <assert.h>
#include <stdarg.h>
#include <string.h>

static char prefix[][20] = {
    "", "", "", "IO error:", "Runtime error:", "Parse error:"};

#define unused(x) ((void)(x))

enum hmr_rc error_io(char *dst, int errnum)
{
    int rc = strerror_r(errnum, dst, HMR_ERROR_SIZE);
    assert(!rc);
    unused(rc);
    return HMR_IOERROR;
}

enum hmr_rc error(enum hmr_rc rc, char *dst, char const *msg)
{
    int n = snprintf(dst, HMR_ERROR_SIZE, "%s %s", prefix[rc], msg);
    assert(0 < n && n < HMR_ERROR_SIZE);
    unused(n);
    return rc;
}

enum hmr_rc error_parse(struct hmr_tok *tok, char const *msg)
{
    int n = snprintf(tok->error, HMR_ERROR_SIZE, "%s %s: line %d",
                     prefix[HMR_PARSEERROR], msg, tok->line.number);
    assert(0 < n && n < HMR_ERROR_SIZE);
    unused(n);
    return HMR_PARSEERROR;
}
