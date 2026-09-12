#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

#include "executor.h"
#include "builtin.h"
#include "token.h"
#include "var.h"

static enum execute_result process_execute_pipe(char *argv_left[], char *argv_right[]) {
    int pipefd[2];

    if (pipe(pipefd) < 0)
        return EXECUTE_ERROR;

    pid_t left_pid = fork();

    if (left_pid < 0)
        return EXECUTE_ERROR;

    if (left_pid == 0) {
        close(pipefd[0]);

        if (dup2(pipefd[1], STDOUT_FILENO) < 0)
            _exit(126);

        close(pipefd[1]);

        execvp(argv_left[0], argv_left);
        _exit(127);
    }

    pid_t right_pid = fork();

    if (right_pid < 0)
        return EXECUTE_ERROR;

    if (right_pid == 0) {
        close(pipefd[1]);

        if (dup2(pipefd[0], STDIN_FILENO) < 0)
            _exit(126);

        close(pipefd[0]);

        execvp(argv_right[0], argv_right);
        _exit(127);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    int left_status;
    int right_status;

    if (waitpid(left_pid, &left_status, 0) < 0)
        return EXECUTE_ERROR;

    if (waitpid(right_pid, &right_status, 0) < 0)
        return EXECUTE_ERROR;

    if (WIFEXITED(right_status) &&
        WEXITSTATUS(right_status) == 127)
        return EXECUTE_UNKNOWN;

    return EXECUTE_OK;
}

static int process_execute(char *argv[], char *input_file, char *output_file, int append, int background, char *temporary_assignment) {
    pid_t pid = fork();

    if (pid < 0)
        return -1;

    if (pid == 0) {
        int fd;

        if (temporary_assignment != NULL) {
            char *equals = strchr(temporary_assignment, '=');

            if (equals != NULL) {
                *equals = '\0';

                if (setenv(
                    temporary_assignment,
                    equals + 1,
                    1
                ) != 0)
                    _exit(126);

                *equals = '=';
            }
        }

        if (input_file != NULL) {
            fd = open(input_file, O_RDONLY);

            if (fd < 0)
                _exit(126);

            if (dup2(fd, STDIN_FILENO) < 0)
                _exit(126);

            close(fd);
        }

        if (output_file != NULL) {
            int flags = O_WRONLY | O_CREAT;

            if (append)
                flags |= O_APPEND;
            else
                flags |= O_TRUNC;

            fd = open(output_file, flags, 0666);

            if (fd < 0)
                _exit(126);

            if (dup2(fd, STDOUT_FILENO) < 0)
                _exit(126);

            close(fd);
        }

        execvp(argv[0], argv);
        _exit(127);
    }

    if (background)
        return 0;

    int status;

    if (waitpid(pid, &status, 0) < 0)
        return -1;

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return 1;
}

static int is_assignment(const char *word) {
    const char *equals = strchr(word, '=');

    if (equals == NULL || equals == word)
        return 0;

    for (const char *p = word; p < equals; p++) {
        if (!((*p >= 'a' && *p <= 'z') ||
              (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') ||
              *p == '_'))
            return 0;
    }

    return 1;
}

static char *expand_word(const char *word, struct variables *vars) {
    size_t len = strlen(word);
    size_t capacity = len + 1;
    size_t out_len = 0;
    char *result = malloc(capacity);

    if (result == NULL)
        return NULL;

    for (size_t i = 0; i < len; ) {
        if (word[i] != '$') {
            if (out_len + 2 > capacity) {
                capacity *= 2;
                result = realloc(result, capacity);

                if (result == NULL)
                    return NULL;
            }

            result[out_len++] = word[i++];
            continue;
        }

        i++;

        size_t start = i;

        while (i < len &&
               ((word[i] >= 'a' && word[i] <= 'z') ||
                (word[i] >= 'A' && word[i] <= 'Z') ||
                (word[i] >= '0' && word[i] <= '9') ||
                word[i] == '_')) {
            i++;
        }

        if (start == i) {
            if (out_len + 2 > capacity) {
                capacity *= 2;
                result = realloc(result, capacity);

                if (result == NULL)
                    return NULL;
            }

            result[out_len++] = '$';
            continue;
        }

        char name[128];
        size_t name_len = i - start;

        if (name_len >= sizeof(name)) {
            free(result);
            return NULL;
        }

        memcpy(name, word + start, name_len);
        name[name_len] = '\0';

        const char *value = var_get(vars, name);

        if (value == NULL)
            value = "";

        size_t value_len = strlen(value);

        while (out_len + value_len + 1 > capacity) {
            capacity *= 2;
            result = realloc(result, capacity);

            if (result == NULL)
                return NULL;
        }

        memcpy(result + out_len, value, value_len);
        out_len += value_len;
    }

    result[out_len] = '\0';

    return result;
}

enum execute_result execute(struct token tokens[], int count, struct variables *vars) {
    char *argv_left[MAX_ARGS];
    char *argv_right[MAX_ARGS];

    int left_argc = 0;
    int right_argc = 0;

    int pipe_index = -1;
    int and_index = -1;
    int background = 0;

    char *temporary_assignment = NULL;

    if (count > 0 &&
        tokens[count - 1].type == TOKEN_BACKGROUND) {
        background = 1;
        count--;
    }

    for (int i = 0; i < count; i++) {
        if (tokens[i].type != TOKEN_WORD)
            continue;

        char *expanded = expand_word(tokens[i].value, vars);

        if (expanded == NULL)
            return EXECUTE_ERROR;

        tokens[i].value = expanded;
    }

    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOKEN_WORD &&
            left_argc == 0 &&
            is_assignment(tokens[i].value)) {
            temporary_assignment = tokens[i].value;
            continue;
        }

        if (tokens[i].type == TOKEN_PARAMETER) {
            if (i + 1 >= count || tokens[i + 1].type != TOKEN_WORD)
                return EXECUTE_ERROR;

            const char *value = var_get(vars, tokens[i + 1].value);

            if (value == NULL)
                value = "";

            tokens[i].value = (char *)value;
            tokens[i].type = TOKEN_WORD;

            for (int j = i + 1; j + 1 < count; j++)
                tokens[j] = tokens[j + 1];

            count--;
        }

        if (tokens[i].type == TOKEN_PIPE) {
            pipe_index = i;
            break;
        }

        if (tokens[i].type == TOKEN_AND_IF) {
            and_index = i;
            break;
        }

        if (tokens[i].type != TOKEN_WORD)
            break;

        if (left_argc >= MAX_ARGS - 1)
            return EXECUTE_ERROR;

        argv_left[left_argc++] = tokens[i].value;
    }

    if (and_index != -1) {
        argv_left[left_argc] = NULL;

        if (left_argc == 0)
            return EXECUTE_ERROR;

        int status = process_execute(
            argv_left,
            NULL,
            NULL,
            0,
            0,
            temporary_assignment
        );

        if (status == 127)
            return EXECUTE_UNKNOWN;

        if (status != 0)
            return EXECUTE_OK;

        return execute(
            &tokens[and_index + 1],
            count - and_index - 1,
            vars
        );
    }

    if (pipe_index != -1) {
        for (int i = pipe_index + 1; i < count; i++) {
            if (tokens[i].type != TOKEN_WORD)
                return EXECUTE_ERROR;

            if (right_argc >= MAX_ARGS - 1)
                return EXECUTE_ERROR;

            argv_right[right_argc++] = tokens[i].value;
        }

        argv_left[left_argc] = NULL;
        argv_right[right_argc] = NULL;

        if (left_argc == 0 || right_argc == 0)
            return EXECUTE_ERROR;

        return process_execute_pipe(
            argv_left,
            argv_right
        );
    }

    argv_left[left_argc] = NULL;

    if (left_argc == 0)
        return EXECUTE_OK;

    int result = builtin_execute(argv_left, vars);

    if (result != -1)
        return result;

    int status = process_execute(
        argv_left,
        NULL,
        NULL,
        0,
        background,
        temporary_assignment
    );

    if (status == 127)
        return EXECUTE_UNKNOWN;

    return EXECUTE_OK;
}
