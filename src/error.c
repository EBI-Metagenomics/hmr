#include "error.h"
#include "aux.h"
#include "bug.h"
#include "hmr/aux.h"
#include "hmr/prof.h"
#include "hmr/token.h"
#include <stdio.h>
#include <string.h>

#define PARSE_MSG "Parse error: "
#define LINE_MSG ": line "

void __error(char *dst, char const *msg);
void __errorl(char *dst, char const *msg, unsigned line);

void error_prof(struct hmr_prof *prof, char const *msg)
{
    __error(prof->error, msg);
}

void error_tok(struct hmr_token *tok, char const *msg)
{
    __errorl(tok->error, msg, tok->line_number);
}

void __error(char *dst, char const *msg)
{
    dst = stpcpy(dst, PARSE_MSG);
    dst = memccpy(dst, msg, '\0', HMR_ERROR_SIZE - sizeof PARSE_MSG);
    BUG(!dst);
}

void __errorl(char *dst, char const *msg, unsigned line)
{
    dst = stpcpy(dst, PARSE_MSG);
    dst = memccpy(dst, msg, '\0',
                  HMR_ERROR_SIZE - sizeof PARSE_MSG - sizeof LINE_MSG - 6);
    BUG(!dst);
    dst = stpcpy(dst - 1, LINE_MSG);
    int n = snprintf(dst, 6, "%d", line);
    BUG(n >= 6);
}
