#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "file-utils.h"

int read_token(FILE *fp, char *seps, char *token)
{
    int len = 0;
    int ch;
    while((ch = fgetc(fp)) != EOF) {
        if (strchr(seps, ch)) {
            if (len != 0) break;
        } else {
            token[len++] = ch;
        }
    }
    token[len] = '\0';
    return len;
}

int peek_token(FILE *fp, char *seps, char *token)
{
    int len;
    fpos_t pos;
    fgetpos(fp, &pos);
    len = read_token(fp, seps, token);
    fsetpos(fp, &pos);
    return len;
}

void expect_token(FILE *fp, char *seps, char *expect)
{
    char tok[255];
    int n = 0;
    n = read_token(fp, seps, tok);
    assert(n < 255);
    assert(strcmp(tok, expect) == 0);
}

