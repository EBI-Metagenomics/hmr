#include "token.h"
#include <string.h>

#define DELIM " \t\r"

#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))
#define STRLEN(x) (ARRLEN(x) - 1)
#define TOKEN(t, s) (t->len == STRLEN(s) && !strncmp(t->line.begin, s, t->len))

static void add_space_before_newline(char line[TOKEN_LINE_MAX]);
static bool next_line(FILE *restrict fd, char line[TOKEN_LINE_MAX],
                      enum hmr_rc *rc);

void token_init(struct token *tok)
{
    tok->id = TOKEN_NEWLINE;
    memset(tok->line, '\0', TOKEN_LINE_MAX);
    tok->value = tok->line;
    tok->rc = HMR_SUCCESS;
}

bool token_next(FILE *restrict fd, struct token *tok)
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
        tok->id = TOKEN_NEWLINE;
    else if (!strcmp(tok->value, "//"))
        tok->id = TOKEN_SLASH;
    else if (!strcmp(tok->value, "HMM"))
        tok->id = TOKEN_HMM;
    else if (!strcmp(tok->value, "COMPO"))
        tok->id = TOKEN_COMPO;
    else
        tok->id = TOKEN_WORD;

    return true;
}

static bool next_line(FILE *restrict fd, char line[TOKEN_LINE_MAX],
                      enum hmr_rc *rc)
{
    *rc = HMR_SUCCESS;

    if (!fgets(line, TOKEN_LINE_MAX - 1, fd))
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

static void add_space_before_newline(char line[TOKEN_LINE_MAX])
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
