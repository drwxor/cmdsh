#include <ctype.h>
#include <stddef.h>

#include "token.h"

int tokenize(char *input, struct token tokens[]) {
    int count = 0;
    char *p = input;

    while (*p != '\0' && count < MAX_TOKENS - 1) {
        while (isspace((unsigned char)*p))
            p++;

        if (*p == '\0')
            break;

        if (*p == '&' && *(p + 1) == '&') {
            tokens[count].type = TOKEN_AND_IF;
            tokens[count].value = "&&";
            count++;
            p += 2;
            continue;
        }

        if (*p == '>' && *(p + 1) == '>') {
            tokens[count].type = TOKEN_REDIRECT_APPEND;
            tokens[count].value = ">>";
            count++;
            p += 2;
            continue;
        }

        if (*p == '>') {
            tokens[count].type = TOKEN_REDIRECT_OUT;
            tokens[count].value = ">";
            count++;
            p++;
            continue;
        }

        if (*p == '<') {
            tokens[count].type = TOKEN_REDIRECT_IN;
            tokens[count].value = "<";
            count++;
            p++;
            continue;
        }

        if (*p == '|') {
            tokens[count].type = TOKEN_PIPE;
            tokens[count].value = "|";
            count++;
            p++;
            continue;
        }

        char *start = p;
        char *write = p;
        char quote = 0;

        while (*p != '\0') {
            if (quote == 0) {
                if (*p == '\'' || *p == '"') {
                    quote = *p;
                    p++;
                    continue;
                }

                if (isspace((unsigned char)*p))
                    break;

                if (*p == '|' ||
                    *p == '>' ||
                    (*p == '&' && *(p + 1) == '&'))
                    break;
            } else {
                if (*p == quote) {
                    quote = 0;
                    p++;
                    continue;
                }
            }

            *write++ = *p++;
        }

        int whitespace = isspace((unsigned char)*p);

        *write = '\0';

        tokens[count].type = TOKEN_WORD;
        tokens[count].value = start;
        count++;

        if (whitespace)
            p++;
    }

    tokens[count].value = NULL;
    return count;
}
