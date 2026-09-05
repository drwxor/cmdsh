#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "token.h"
#include "var.h"

#define MAX_ARGS 64

enum execute_result {
    EXECUTE_OK,
    EXECUTE_EXIT,
    EXECUTE_UNKNOWN,
    EXECUTE_ERROR
};

enum execute_result execute(
    struct token tokens[],
    int count,
    struct variables *vars
);

#endif // EXECUTOR_H
