#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <limits.h>
#include <unistd.h>

#include "config.h"
#include "executor.h"
#include "token.h"
#include "lineedit.h"
#include "var.h"

static volatile sig_atomic_t got_sigint = 0;

static void reap_children(void) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

static void handle_sigint(int sig) {
    (void)sig;
    got_sigint = 1;
}

int main(int argc, char *argv[], char *envp[]) {
    char input[1024];
    struct token tokens[MAX_TOKENS];
    struct variables vars = {0};

    if (var_import(&vars, envp) != 0)
        return 1;

    char shell_path[PATH_MAX];

    ssize_t shell_len = readlink(
        "/proc/self/exe",
        shell_path,
        sizeof(shell_path) - 1
    );

    if (shell_len >= 0) {
        shell_path[shell_len] = '\0';

        if (var_set(&vars, "SHELL", shell_path) != 0)
            return 1;

        if (var_export(&vars, "SHELL") != 0)
            return 1;
    }

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

        enum execute_result result = execute(tokens, count, &vars);

        if (result == EXECUTE_EXIT)
            break;

        if (result == EXECUTE_UNKNOWN)
            fprintf(stderr, "%s: command not found\n", tokens[0].value);
    }

    var_free(&vars);
    return 0;
}
