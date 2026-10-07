#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "redir.h"

RedirCommand *parse_redir_line(const char *line) {
    RedirCommand *cmd = (RedirCommand *)calloc(1, sizeof(RedirCommand));
    if (!line) return cmd;

    char *copy = strdup(line);
    char *saveptr;
    char *tok = strtok_r(copy, " \t", &saveptr);

    while (tok) {
        if (strcmp(tok, "<") == 0) {
            char *file = strtok_r(NULL, " \t", &saveptr);
            if (file) cmd->input_file = strdup(file);
        } else if (strcmp(tok, ">") == 0) {
            char *file = strtok_r(NULL, " \t", &saveptr);
            if (file) cmd->output_file = strdup(file);
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

int execute_with_redirection(const RedirCommand *cmd) {
    if (!cmd || cmd->argc == 0 || !cmd->argv || !cmd->argv[0]) return 0;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* Child Process */

        /* 1. Handle Input Redirection '<' */
        if (cmd->input_file) {
            int in_fd = open(cmd->input_file, O_RDONLY);
            if (in_fd < 0) {
                fprintf(stderr, "myshell: %s: %s\n", cmd->input_file, strerror(errno));
                exit(1);
            }
            if (dup2(in_fd, STDIN_FILENO) < 0) {
                perror("dup2 stdin");
                close(in_fd);
                exit(1);
            }
            close(in_fd);
        }

        /* 2. Handle Output Redirection '>' */
        if (cmd->output_file) {
            int out_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) {
                fprintf(stderr, "myshell: %s: %s\n", cmd->output_file, strerror(errno));
                exit(1);
            }
            if (dup2(out_fd, STDOUT_FILENO) < 0) {
                perror("dup2 stdout");
                close(out_fd);
                exit(1);
            }
            close(out_fd);
        }

        /* 3. Execute binary */
        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "myshell: %s: %s\n", cmd->argv[0], strerror(errno));
        exit(127);
    } else {
        /* Parent Process */
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
}

void free_redir_command(RedirCommand *cmd) {
    if (!cmd) return;
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }
    if (cmd->argv) free(cmd->argv);
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
    free(cmd);
}
