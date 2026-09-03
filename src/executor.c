#include <stddef.h>

#include "executor.h"

#include "builtin.h"
#include "cprocess.h"

int execute(struct token tokens[], int count) {
    char *argv[MAX_ARGS];
    int argc = 0;

    for (int i = 0; i < count; i++) {
        if (tokens[i].type != TOKEN_WORD)
            break;

        if (argc >= MAX_ARGS - 1)
            break;

        argv[argc++] = tokens[i].value;
    }

    argv[argc] = NULL;

    if (argc == 0)
        return EXECUTE_OK;

    int result = builtin_execute(argv);

    if (result != -1)
        return result;

    return process_execute(argv);
}
