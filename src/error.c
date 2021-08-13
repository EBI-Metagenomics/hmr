#include "error.h"
#include "aux.h"
#include "bug.h"
#include "hmr/aux.h"
#include "hmr/prof.h"
#include "hmr/tok.h"
#include <stdio.h>
#include <string.h>

#define PARSE_MSG "Parse error: "
#define LINE_MSG ": line "

void error(char *dst, char const *msg);
void errorl(char *dst, char const *msg, unsigned line);

enum hmr_rc __error_parse_prof(struct hmr_prof *prof, char const *msg)
{
    error(prof->error, msg);
    return HMR_PARSEERROR;
}

enum hmr_rc __error_parse_tok(struct hmr_tok *tok, char const *msg)
{
    errorl(tok->error, msg, tok->line_number);
    return HMR_PARSEERROR;
}

enum hmr_rc error_io(char *dst, int errnum)
{
    strerror_r(errnum, dst, HMR_ERROR_SIZE);
    return HMR_IOERROR;
}

enum hmr_rc error_runtime(char *dst, char const func[], char const *msg)
{
    error(dst, msg);
    return HMR_RUNTIMEERROR;
}

void error(char *dst, char const *msg)
{
    dst = stpcpy(dst, PARSE_MSG);
    dst = memccpy(dst, msg, '\0', HMR_ERROR_SIZE - sizeof PARSE_MSG);
    BUG(!dst);
}

void errorl(char *dst, char const *msg, unsigned line)
{
    dst = stpcpy(dst, PARSE_MSG);
    dst = memccpy(dst, msg, '\0',
                  HMR_ERROR_SIZE - sizeof PARSE_MSG - sizeof LINE_MSG - 6);
    BUG(!dst);
    dst = stpcpy(dst - 1, LINE_MSG);
    int n = snprintf(dst, 6, "%d", line);
    BUG(n >= 6);
}
