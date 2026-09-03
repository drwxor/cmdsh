#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "token.h"

#define MAX_ARGS 64

#define EXECUTE_OK 0
#define EXECUTE_EXIT 1

int execute(struct token tokens[], int count);

#endif // EXECUTOR_H
