#ifndef VAR_H
#define VAR_H

#include <stddef.h>

struct variable {
    char *name;
    char *value;
    int exported;
};

struct variables {
    struct variable *items;
    size_t count;
    size_t capacity;
};

int var_set(struct variables *vars, const char *name, const char *value);
const char *var_get(const struct variables *vars, const char *name);
int var_unset(struct variables *vars, const char *name);
int var_export(struct variables *vars, const char *name);
void var_free(struct variables *vars);

#endif // VAR_H
