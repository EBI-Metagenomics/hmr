#include "token.h"
#include "hmr/token.h"
#include <string.h>

#define DELIM " \t\r"

static void add_space_before_newline(char line[HMR_TOKEN_LINE_MAX]);
static bool next_line(FILE *restrict fd, char line[HMR_TOKEN_LINE_MAX],
                      enum hmr_rc *rc);

void token_init(struct hmr_token *tok)
{
    tok->id = HMR_TOKEN_NEWLINE;
    memset(tok->line, '\0', HMR_TOKEN_LINE_MAX);
    tok->value = tok->line;
    tok->rc = HMR_SUCCESS;
}

bool token_next(FILE *restrict fd, struct hmr_token *tok)
{
    tok->rc = HMR_SUCCESS;

    if (!(tok->value = strtok(NULL, DELIM)))
    {
        if (!next_line(fd, tok->line, &tok->rc))
            return false;
        tok->value = strtok(tok->line, DELIM);

        if (!tok->value)
        {
            tok->rc = HMR_PARSEERROR;
            return false;
        }
    }

    if (!strcmp(tok->value, "\n"))
        tok->id = HMR_TOKEN_NEWLINE;
    else if (!strcmp(tok->value, "//"))
        tok->id = HMR_TOKEN_SLASH;
    else if (!strcmp(tok->value, "HMM"))
        tok->id = HMR_TOKEN_HMM;
    else if (!strcmp(tok->value, "COMPO"))
        tok->id = HMR_TOKEN_COMPO;
    else
        tok->id = HMR_TOKEN_WORD;

    return true;
}

static bool next_line(FILE *restrict fd, char line[HMR_TOKEN_LINE_MAX],
                      enum hmr_rc *rc)
{
    *rc = HMR_SUCCESS;

    if (!fgets(line, HMR_TOKEN_LINE_MAX - 1, fd))
    {
        if (feof(fd))
            return false;

        perror("fgets() failed");
        clearerr(fd);
        *rc = HMR_IOERROR;
        return false;
    }

    add_space_before_newline(line);
    return true;
}

static void add_space_before_newline(char line[HMR_TOKEN_LINE_MAX])
{
    unsigned n = (unsigned)strlen(line);
    if (n > 0)
    {
        if (line[n - 1] == '\n')
        {
            line[n - 1] = ' ';
            line[n] = '\n';
            line[n + 1] = '\0';
        }
        else
        {
            line[n - 1] = '\n';
            line[n] = '\0';
        }
    }
}
