#ifndef TOKEN_H
#define TOKEN_H

#define MAX_TOKENS 64

enum token_type {
    TOKEN_WORD,
    TOKEN_AND_IF,
    TOKEN_REDIRECT_OUT,
    TOKEN_REDIRECT_APPEND,
    TOKEN_PIPE,
};

struct token {
    enum token_type type;
    char *value;
};

int tokenize(char *input, struct token tokens[]);

#endif // TOKEN_H
