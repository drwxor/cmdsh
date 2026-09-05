#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>

#include "config.h"
#include "executor.h"
#include "token.h"
#include "lineedit.h"

static volatile sig_atomic_t got_sigint = 0;

static void reap_children(void) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

static void handle_sigint(int sig) {
    (void)sig;
    got_sigint = 1;
}

int main(void) {
    char input[1024];
    struct token tokens[MAX_TOKENS];

    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    for (;;) {
        reap_children();

        got_sigint = 0;

        char *prompt;

        if (geteuid() == 0)
            prompt = ROOT_PROMPT;
        else
            prompt = USER_PROMPT;

        fputs(prompt, stdout);
        fflush(stdout);

        if (line_read(input, prompt, sizeof(input)) < 0) {
            if (got_sigint) {
                clearerr(stdin);
                fputc('\n', stdout);
                continue;
            }

            break;
        }

        if (input[0] == '\n')
            continue;

        int count = tokenize(input, tokens);

        enum execute_result result = execute(tokens, count);

        if (result == EXECUTE_EXIT)
            break;

        if (result == EXECUTE_UNKNOWN)
            fprintf(stderr, "%s: command not found\n", tokens[0].value);
    }

    return 0;
}
