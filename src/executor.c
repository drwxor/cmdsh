#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "executor.h"
#include "builtin.h"

static int process_execute(char *argv[]) {
    pid_t pid = fork();

    if (pid < 0)
        return -1;

    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }

    int status;

    if (waitpid(pid, &status, 0) < 0)
        return -1;

    return status;
}

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
