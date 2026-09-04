#include <stdio.h>

#include "config.h"
#include "executor.h"
#include "token.h"

int main(void) {
    char input[1024];
    struct token tokens[MAX_TOKENS];

    for (;;) {
        fputs(PROMPT, stdout);
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        if (input[0] == '\n')
            continue;

        int count = tokenize(input, tokens);

        int result = execute(tokens, count);

        if (result == EXECUTE_EXIT)
            break;
    }

    return 0;
}
