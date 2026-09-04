#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "executor.h"
#include "builtin.h"

static enum execute_result process_execute_pipe(
    char *argv_left[],
    char *argv_right[]
) {
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

static int process_execute(
    char *argv[],
    char *input_file,
    char *output_file,
    int append
) {
    pid_t pid = fork();

    if (pid < 0)
        return -1;

    if (pid == 0) {
        int fd;

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

    int status;

    if (waitpid(pid, &status, 0) < 0)
        return -1;

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return 1;
}

enum execute_result execute(struct token tokens[], int count) {
    char *argv_left[MAX_ARGS];
    char *argv_right[MAX_ARGS];

    int left_argc = 0;
    int right_argc = 0;

    int pipe_index = -1;
    int and_index = -1;

    for (int i = 0; i < count; i++) {
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
            0
        );

        if (status == 127)
            return EXECUTE_UNKNOWN;

        if (status != 0)
            return EXECUTE_OK;

        return execute(
            &tokens[and_index + 1],
            count - and_index - 1
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

    int result = builtin_execute(argv_left);

    if (result != -1)
        return result;

    int status = process_execute(
        argv_left,
        NULL,
        NULL,
        0
    );

    if (status == 127)
        return EXECUTE_UNKNOWN;

    return EXECUTE_OK;
}
