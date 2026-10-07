#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "redir_ext.h"

RedirExtCommand *parse_redir_ext_line(const char *line) {
    RedirExtCommand *cmd = (RedirExtCommand *)calloc(1, sizeof(RedirExtCommand));
    if (!line) return cmd;

    char *copy = strdup(line);
    char *saveptr;
    char *tok = strtok_r(copy, " \t", &saveptr);

    while (tok) {
        if (strcmp(tok, ">>") == 0) {
            char *file = strtok_r(NULL, " \t", &saveptr);
            if (file) cmd->append_out_file = strdup(file);
        } else if (strcmp(tok, "2>") == 0) {
            char *file = strtok_r(NULL, " \t", &saveptr);
            if (file) cmd->err_file = strdup(file);
        } else {
            cmd->argv = (char **)realloc(cmd->argv, (cmd->argc + 2) * sizeof(char *));
            cmd->argv[cmd->argc++] = strdup(tok);
            cmd->argv[cmd->argc] = NULL;
        }
        tok = strtok_r(NULL, " \t", &saveptr);
    }

    free(copy);
    return cmd;
}

int execute_redir_ext(const RedirExtCommand *cmd) {
    if (!cmd || cmd->argc == 0 || !cmd->argv || !cmd->argv[0]) return 0;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* Child Process */

        /* 1. Append Redirection '>>' (STDOUT_FILENO = 1) */
        if (cmd->append_out_file) {
            int out_fd = open(cmd->append_out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (out_fd < 0) {
                fprintf(stderr, "myshell: %s: %s\n", cmd->append_out_file, strerror(errno));
                exit(1);
            }
            if (dup2(out_fd, STDOUT_FILENO) < 0) {
                perror("dup2 stdout append");
                close(out_fd);
                exit(1);
            }
            close(out_fd);
        }

        /* 2. Stderr Redirection '2>' (STDERR_FILENO = 2) */
        if (cmd->err_file) {
            int err_fd = open(cmd->err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (err_fd < 0) {
                fprintf(stderr, "myshell: %s: %s\n", cmd->err_file, strerror(errno));
                exit(1);
            }
            if (dup2(err_fd, STDERR_FILENO) < 0) {
                perror("dup2 stderr");
                close(err_fd);
                exit(1);
            }
            close(err_fd);
        }

        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "myshell: %s: %s\n", cmd->argv[0], strerror(errno));
        exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
}

void free_redir_ext(RedirExtCommand *cmd) {
    if (!cmd) return;
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }
    if (cmd->argv) free(cmd->argv);
    if (cmd->append_out_file) free(cmd->append_out_file);
    if (cmd->err_file) free(cmd->err_file);
    free(cmd);
}
