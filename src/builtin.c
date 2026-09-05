#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "executor.h"
#include "var.h"

int builtin_exit(char *argv[], struct variables *vars) {
    (void)argv;
    (void)vars;

    return EXECUTE_EXIT;
}

int builtin_cd(char *argv[], struct variables *vars) {
    (void)vars;

    if (argv[1] == NULL)
        return EXECUTE_OK;

    if (chdir(argv[1]) != 0) {
        perror("cd");
        return EXECUTE_ERROR;
    }

    return EXECUTE_OK;
}

int builtin_export(char *argv[], struct variables *vars) {
    if (argv[1] == NULL)
        return EXECUTE_OK;

    char *equals = strchr(argv[1], '=');

    if (equals != NULL) {
        *equals = '\0';

        if (var_set(vars, argv[1], equals + 1) != 0)
            return EXECUTE_ERROR;

        if (var_export(vars, argv[1]) != 0)
            return EXECUTE_ERROR;

        return EXECUTE_OK;
    }

    if (var_export(vars, argv[1]) != 0) {
        fprintf(stderr, "export: %s: not a variable\n", argv[1]);
        return EXECUTE_ERROR;
    }

    return EXECUTE_OK;
}

int builtin_unset(char *argv[], struct variables *vars) {
    if (argv[1] == NULL)
        return EXECUTE_OK;

    if (var_unset(vars, argv[1]) != 0)
        return EXECUTE_ERROR;

    return EXECUTE_OK;
}

static const struct builtin builtins[] = {
    { "exit",   builtin_exit },
    { "cd",     builtin_cd },
    { "export", builtin_export },
    { "unset",  builtin_unset },
    { NULL,     NULL }
};

int builtin_execute(char *argv[], struct variables *vars) {
    int i;

    for (i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0)
            return builtins[i].function(argv, vars);
    }

    return -1;
}
