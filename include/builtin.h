#ifndef BUILTIN_H
#define BUILTIN_H

struct builtin {
    const char *name;
    int (*function)(char *argv[]);
};

int builtin_execute(char *argv[]);

#endif // BUILTIN_H
