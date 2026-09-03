#include <stdio.h>
#include <string.h>

#include "builtin.h"

#include "executor.h"
#include "cprocess.h"

int builtin_exit(char *argv[]) {
    (void)argv;
    return EXECUTE_EXIT;
}

int builtin_cd(char *argv[]) {
    if (argv[1] == NULL)
        return EXECUTE_OK;

    if (process_chdir(argv[1]) != 0) {
        perror("cd");
        return EXECUTE_OK;
    }

    return EXECUTE_OK;
}

static const struct builtin builtins[] = {
    { "exit", builtin_exit },
    { "cd", builtin_cd },
    { NULL, NULL }
};

int builtin_execute(char *argv[]) {
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0)
            return builtins[i].function(argv);
    }

    return -1;
}
