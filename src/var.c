#include "var.h"

#include <stdlib.h>
#include <string.h>

static char *var_strdup(const char *str) {
    size_t len = strlen(str) + 1;
    char *copy = malloc(len);

    if (copy == NULL)
        return NULL;

    memcpy(copy, str, len);

    return copy;
}

static int var_find(const struct variables *vars, const char *name) {
    size_t i;

    if (vars == NULL || name == NULL)
        return -1;

    for (i = 0; i < vars->count; i++) {
        if (strcmp(vars->items[i].name, name) == 0)
            return (int)i;
    }

    return -1;
}

int var_set(struct variables *vars, const char *name, const char *value) {
    int index;
    struct variable *items;
    size_t new_capacity;

    if (vars == NULL || name == NULL || value == NULL)
        return -1;

    index = var_find(vars, name);

    if (index >= 0) {
        char *new_value;

        new_value = var_strdup(value);
        if (new_value == NULL)
            return -1;

        free(vars->items[index].value);
        vars->items[index].value = new_value;

        return 0;
    }

    if (vars->count == vars->capacity) {
        if (vars->capacity == 0)
            new_capacity = 8;
        else
            new_capacity = vars->capacity * 2;

        items = realloc(
            vars->items,
            new_capacity * sizeof(*vars->items)
        );

        if (items == NULL)
            return -1;

        vars->items = items;
        vars->capacity = new_capacity;
    }

    vars->items[vars->count].name = var_strdup(name);
    vars->items[vars->count].value = var_strdup(value);

    if (vars->items[vars->count].name == NULL ||
        vars->items[vars->count].value == NULL) {
        free(vars->items[vars->count].name);
        free(vars->items[vars->count].value);

        vars->items[vars->count].name = NULL;
        vars->items[vars->count].value = NULL;

        return -1;
    }

    vars->items[vars->count].exported = 0;
    vars->count++;

    return 0;
}

const char *var_get(const struct variables *vars, const char *name) {
    int index;

    index = var_find(vars, name);

    if (index < 0)
        return NULL;

    return vars->items[index].value;
}

int var_unset(struct variables *vars, const char *name) {
    int index;
    size_t i;

    if (vars == NULL || name == NULL)
        return -1;

    index = var_find(vars, name);

    if (index < 0)
        return 0;

    free(vars->items[index].name);
    free(vars->items[index].value);

    for (i = (size_t)index; i + 1 < vars->count; i++)
        vars->items[i] = vars->items[i + 1];

    vars->count--;

    return 0;
}

int var_export(struct variables *vars, const char *name) {
    int index;

    if (vars == NULL || name == NULL)
        return -1;

    index = var_find(vars, name);

    if (index < 0)
        return -1;

    vars->items[index].exported = 1;

    return 0;
}

void var_free(struct variables *vars) {
    size_t i;

    if (vars == NULL)
        return;

    for (i = 0; i < vars->count; i++) {
        free(vars->items[i].name);
        free(vars->items[i].value);
    }

    free(vars->items);

    vars->items = NULL;
    vars->count = 0;
    vars->capacity = 0;
}
