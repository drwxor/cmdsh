#ifndef BUILTIN_H
#define BUILTIN_H

#include "var.h"

struct builtin {
    const char *name;
    int (*function)(char *argv[], struct variables *vars);
};

int builtin_execute(char *argv[], struct variables *vars);

#endif // BUILTIN_H
