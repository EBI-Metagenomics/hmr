#include "token.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TOKEN_MAX 64
#define DELIM " \t\r"

#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))
#define STRLEN(x) (ARRLEN(x) - 1)
#define TOKEN(t, s) (t->len == STRLEN(s) && !strncmp(t->line.begin, s, t->len))

static bool next_line(FILE *restrict fd, char line[TOKEN_LINE_MAX], int *rc);

void token_init(struct token *token)
{
    token->id = NEWLINE;
    memset(token->line.data, '\0', TOKEN_LINE_MAX);
    token->line.begin = token->line.data;
    token->line.end = token->line.data;
    token->len = 0;
}

bool token_next(FILE *restrict fd, struct token *t, int *rc)
{
    *rc = 0;
    if (t->line.end[0] == '\0')
    {
        if (!next_line(fd, t->line.data, rc))
            return false;
        t->line.begin = t->line.data;
        t->line.end = t->line.data;
    }
    t->line.begin = t->line.end + strspn(t->line.end, DELIM);
    t->line.end = t->line.begin + strcspn(t->line.begin, DELIM "\n");
    if (*t->line.begin == '\n')
        t->line.end++;
    t->len = (unsigned)(t->line.end - t->line.begin);

    if (TOKEN(t, "\n"))
        t->id = NEWLINE;
    else if (TOKEN(t, "//"))
        t->id = SLASH;
    else if (TOKEN(t, "HMM"))
        t->id = HMM;
    else
        t->id = WORD;

    return true;
}

static bool next_line(FILE *restrict fd, char line[TOKEN_LINE_MAX], int *rc)
{
    *rc = 0;
    char *buff = fgets(line, TOKEN_LINE_MAX, fd);
    if (!buff)
    {
        if (feof(fd))
            return false;

        if (ferror(fd))
        {
            perror("fgets() failed");
            clearerr(fd);
            *rc = 1;
            return false;
        }

        line[0] = '\0';
    }

    return true;
}

#if 0
int main(int argc, char *argv[])
{
    enum token_state state = BEGIN;
    struct token token;
    token_init(&token);

    FILE *restrict fd = fopen(argv[1], "r");

    int rc = 0;
    while (next_token(fd, &token, &rc))
    {
        state = step(state, &token);
    }

    printf("FINAL State: %s\n", name[state]);

    fclose(fd);

    return rc;
}
#endif
