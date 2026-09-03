#include "process.h"

#ifdef _WIN32

#include <process.h>
#include <direct.h>

int process_execute(char *argv[]) {
    return _spawnvp(_P_WAIT, argv[0], (const char *const *)argv);
}

int process_chdir(const char *path) {
    return _chdir(path);
}

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int process_execute(char *argv[]) {
    pid_t pid = fork();

    if (pid < 0)
        return -1;

    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }

    int status;

    if (waitpid(pid, &status, 0) < 0)
        return -1;

    return status;
}

int process_chdir(const char *path) {
    return chdir(path);
}

#endif
