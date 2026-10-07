#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "pipe_exec.h"

int execute_pipeline(CommandArgv *commands, int num_commands) {
    if (num_commands <= 0 || !commands) return 0;

    /* Handle single command without pipe */
    if (num_commands == 1) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        }
        if (pid == 0) {
            execvp(commands[0].argv[0], commands[0].argv);
            perror("execvp");
            exit(127);
        }
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }

    /* Allocate pipe descriptor array: (num_commands - 1) pipes */
    int num_pipes = num_commands - 1;
    int (*pipefds)[2] = malloc(sizeof(int[2]) * num_pipes);
    if (!pipefds) {
        perror("malloc");
        return -1;
    }

    /* Create all pipes */
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipefds[i]) == -1) {
            perror("pipe");
            free(pipefds);
            return -1;
        }
    }

    pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * num_commands);
    if (!pids) {
        perror("malloc");
        free(pipefds);
        return -1;
    }

    /* Launch each command in pipeline */
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            break;
        }

        if (pids[i] == 0) {
            /* Child process */

            /* If not first command, redirect STDIN from previous pipe read end */
            if (i > 0) {
                if (dup2(pipefds[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2 stdin");
                    exit(1);
                }
            }

            /* If not last command, redirect STDOUT to current pipe write end */
            if (i < num_commands - 1) {
                if (dup2(pipefds[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 stdout");
                    exit(1);
                }
            }

            /* Close ALL pipe descriptors in child */
            for (int j = 0; j < num_pipes; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            /* Execute command */
            execvp(commands[i].argv[0], commands[i].argv);
            fprintf(stderr, "myshell: %s: %s\n", commands[i].argv[0], strerror(errno));
            exit(127);
        }
    }

    /* Parent process: MUST close all pipe ends so readers see EOF! */
    for (int j = 0; j < num_pipes; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }
    free(pipefds);

    /* Synchronize and wait for all child processes */
    int last_exit_status = 0;
    for (int i = 0; i < num_commands; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) != -1) {
            if (i == num_commands - 1 && WIFEXITED(status)) {
                last_exit_status = WEXITSTATUS(status);
            }
        }
    }
    free(pids);

    return last_exit_status;
}
