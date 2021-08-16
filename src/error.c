#include "error.h"
#include "aux.h"
#include "die.h"
#include "hmr/aux.h"
#include "hmr/prof.h"
#include "hmr/tok.h"
#include <stdarg.h>

#define PARSE_ERROR "Parse error: "
#define RUNTIME_ERROR "Runtime error: "
#define LINE ": line"

static inline int copy_fmt(int dst_size, char *dst, char const *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(dst, dst_size, fmt, ap);
    va_end(ap);
    if (n < 0)
        die();
    return n;
}

static inline int copy_ap(int dst_size, char *dst, char const *fmt, va_list ap)
{
    int n = vsnprintf(dst, dst_size, fmt, ap);
    if (n < 0)
        die();
    return n;
}

enum hmr_rc error_io(char *dst, int errnum)
{
    if (strerror_r(errnum, dst, HMR_ERROR_SIZE))
        die();
    return HMR_IOERROR;
}

enum hmr_rc error_runtime(char *dst, char const *fmt, ...)
{
    int n = copy_fmt(HMR_ERROR_SIZE, dst, RUNTIME_ERROR);
    va_list ap;
    va_start(ap, fmt);
    copy_ap(HMR_ERROR_SIZE - n, dst + n, fmt, ap);
    va_end(ap);
    return HMR_RUNTIMEERROR;
}

enum hmr_rc __error_parse_prof(struct hmr_prof *prof, char const *fmt, ...)
{
    int n = copy_fmt(HMR_ERROR_SIZE, prof->error, PARSE_ERROR);
    va_list ap;
    va_start(ap, fmt);
    copy_fmt(HMR_ERROR_SIZE - n, prof->error + n, fmt, ap);
    va_end(ap);
    return HMR_PARSEERROR;
}

enum hmr_rc __error_parse_tok(struct hmr_tok *tok, char const *fmt, ...)
{
    int n = copy_fmt(HMR_ERROR_SIZE, tok->error, PARSE_ERROR);
    va_list ap;
    va_start(ap, fmt);
    n += copy_ap(HMR_ERROR_SIZE - n, tok->error + n, fmt, ap);
    va_end(ap);
    copy_fmt(HMR_ERROR_SIZE - n, tok->error + n, "%s %d", LINE,
             tok->line_number);
    return HMR_PARSEERROR;
}
