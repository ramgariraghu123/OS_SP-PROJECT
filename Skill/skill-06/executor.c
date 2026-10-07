#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "executor.h"

int execute_command(ParsedArgs *args) {
    if (!args || args->argc == 0 || !args->argv || !args->argv[0]) {
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* Child process: launch program with arguments */
        execvp(args->argv[0], args->argv);

        /* If execvp returns, an error occurred */
        if (errno == ENOENT) {
            fprintf(stderr, "myshell: %s: command not found\n", args->argv[0]);
            exit(127);
        } else if (errno == EACCES) {
            fprintf(stderr, "myshell: %s: Permission denied\n", args->argv[0]);
            exit(126);
        } else {
            fprintf(stderr, "myshell: %s: %s\n", args->argv[0], strerror(errno));
            exit(1);
        }
    } else {
        /* Parent process: monitor child execution */
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return -1;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            printf("[Process terminated by signal %d]\n", WTERMSIG(status));
            return 128 + WTERMSIG(status);
        }
        return 0;
    }
}
