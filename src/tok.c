#include "hmr/tok.h"
#include "error.h"
#include "hmr/error.h"
#include "tok.h"
#include <string.h>

#define DELIM " \t\r"

static void add_space_before_newline(char line[HMR_TOK_LINE_MAX]);
static enum hmr_rc next_line(FILE *restrict fd, char error[HMR_ERROR_SIZE],
                             char line[HMR_TOK_LINE_MAX]);

void tok_init(struct hmr_tok *tok, char *error)
{
    tok->id = HMR_TOK_NEWLINE;
    memset(tok->line, '\0', HMR_TOK_LINE_MAX);
    tok->line_number = 0;
    tok->consumed_line = true;
    tok->ptr = NULL;
    tok->value = tok->line;
    tok->error = error;
}

enum hmr_rc tok_next(FILE *restrict fd, struct hmr_tok *tok)
{
    enum hmr_rc rc = HMR_SUCCESS;

    if (tok->consumed_line)
    {
        if ((rc = next_line(fd, tok->error, tok->line)))
        {
            if (rc == HMR_ENDFILE)
            {
                tok->value = NULL;
                tok->id = HMR_TOK_EOF;
                tok->line[0] = '\0';
                return HMR_SUCCESS;
            }
            return rc;
        }
        tok->value = strtok_r(tok->line, DELIM, &tok->ptr);
        tok->line_number++;

        if (!tok->value)
            return HMR_PARSEERROR;
    }
    else
        tok->value = strtok_r(NULL, DELIM, &tok->ptr);

    if (!strcmp(tok->value, "\n"))
        tok->id = HMR_TOK_NEWLINE;
    else if (!strcmp(tok->value, "//"))
        tok->id = HMR_TOK_SLASH;
    else if (!strcmp(tok->value, "HMM"))
        tok->id = HMR_TOK_HMM;
    else if (!strcmp(tok->value, "COMPO"))
        tok->id = HMR_TOK_COMPO;
    else
        tok->id = HMR_TOK_WORD;

    tok->consumed_line = tok->id == HMR_TOK_NEWLINE;

    return HMR_SUCCESS;
}

static enum hmr_rc next_line(FILE *restrict fd, char error[HMR_ERROR_SIZE],
                             char line[HMR_TOK_LINE_MAX])
{
    if (!fgets(line, HMR_TOK_LINE_MAX - 1, fd))
    {
        if (feof(fd))
            return HMR_ENDFILE;

        return error_io(error, ferror(fd));
    }

    add_space_before_newline(line);
    return HMR_SUCCESS;
}

static void add_space_before_newline(char line[HMR_TOK_LINE_MAX])
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
