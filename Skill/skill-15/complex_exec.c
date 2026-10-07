#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include "complex_exec.h"

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

ComplexPipeline *parse_complex_command(const char *cmdline) {
    ComplexPipeline *cp = (ComplexPipeline *)calloc(1, sizeof(ComplexPipeline));
    if (!cmdline) return cp;

    char *copy = strdup(cmdline);
    char *saveptr;
    char *stage_str = strtok_r(copy, "|", &saveptr);

    while (stage_str) {
        char *cleaned = trim(stage_str);
        if (strlen(cleaned) == 0) {
            stage_str = strtok_r(NULL, "|", &saveptr);
            continue;
        }

        cp->stages = (ComplexStage *)realloc(cp->stages, (cp->num_stages + 1) * sizeof(ComplexStage));
        ComplexStage *st = &cp->stages[cp->num_stages];
        memset(st, 0, sizeof(ComplexStage));

        char *tok_save;
        char *tok = strtok_r(cleaned, " \t", &tok_save);
        while (tok) {
            if (strcmp(tok, "<") == 0) {
                char *file = strtok_r(NULL, " \t", &tok_save);
                if (file) st->input_file = strdup(file);
            } else if (strcmp(tok, ">") == 0) {
                char *file = strtok_r(NULL, " \t", &tok_save);
                if (file) {
                    st->output_file = strdup(file);
                    st->append_mode = 0;
                }
            } else if (strcmp(tok, ">>") == 0) {
                char *file = strtok_r(NULL, " \t", &tok_save);
                if (file) {
                    st->output_file = strdup(file);
                    st->append_mode = 1;
                }
            } else if (strcmp(tok, "2>") == 0) {
                char *file = strtok_r(NULL, " \t", &tok_save);
                if (file) st->err_file = strdup(file);
            } else if (strcmp(tok, "2>&1") == 0) {
                st->merge_err_to_out = 1;
            } else if (strcmp(tok, "&>") == 0) {
                char *file = strtok_r(NULL, " \t", &tok_save);
                if (file) {
                    st->output_file = strdup(file);
                    st->merge_err_to_out = 1;
                }
            } else {
                st->argv = (char **)realloc(st->argv, (st->argc + 2) * sizeof(char *));
                st->argv[st->argc++] = strdup(tok);
                st->argv[st->argc] = NULL;
            }
            tok = strtok_r(NULL, " \t", &tok_save);
        }

        cp->num_stages++;
        stage_str = strtok_r(NULL, "|", &saveptr);
    }

    free(copy);
    return cp;
}

void print_execution_plan(const ComplexPipeline *cp) {
    if (!cp || cp->num_stages == 0) {
        printf("  [Execution Plan]: Empty pipeline\n");
        return;
    }

    printf("\n=== Comprehensive Pipeline & Redirection Execution Plan ===\n");
    printf("Total Pipeline Stages: %d\n", cp->num_stages);

    for (int i = 0; i < cp->num_stages; i++) {
        ComplexStage *s = &cp->stages[i];
        printf("Stage [%d]: %s\n", i, s->argv && s->argv[0] ? s->argv[0] : "(none)");
        printf("  - Args: ");
        for (int a = 0; a < s->argc; a++) printf("[%s] ", s->argv[a]);
        printf("\n");

        if (i == 0 && s->input_file) {
            printf("  - STDIN: Redirected from file '%s' (O_RDONLY)\n", s->input_file);
        } else if (i > 0) {
            printf("  - STDIN: Connected from upstream Pipe [%d] read-end\n", i - 1);
        } else {
            printf("  - STDIN: Default terminal stdin\n");
        }

        if (i < cp->num_stages - 1) {
            printf("  - STDOUT: Connected to downstream Pipe [%d] write-end\n", i);
        } else if (s->output_file) {
            printf("  - STDOUT: Redirected to file '%s' (%s)\n",
                   s->output_file, s->append_mode ? "O_APPEND" : "O_TRUNC");
        } else {
            printf("  - STDOUT: Default terminal stdout\n");
        }

        if (s->err_file) {
            printf("  - STDERR: Redirected to file '%s'\n", s->err_file);
        } else if (s->merge_err_to_out) {
            printf("  - STDERR: Merged into STDOUT via dup2(1, 2) (2>&1)\n");
        } else {
            printf("  - STDERR: Default terminal stderr\n");
        }
    }
    printf("============================================================\n\n");
}

int execute_complex_pipeline(const ComplexPipeline *cp) {
    if (!cp || cp->num_stages == 0) return 0;

    int num_pipes = cp->num_stages - 1;
    int (*pipefds)[2] = NULL;
    if (num_pipes > 0) {
        pipefds = malloc(sizeof(int[2]) * num_pipes);
        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipefds[i]) < 0) {
                perror("pipe");
                free(pipefds);
                return -1;
            }
        }
    }

    pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * cp->num_stages);

    for (int i = 0; i < cp->num_stages; i++) {
        ComplexStage *s = &cp->stages[i];

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            break;
        }

        if (pids[i] == 0) {
            /* Child process: configure streams */

            /* 1. Pipe STDIN */
            if (i > 0) {
                dup2(pipefds[i - 1][0], STDIN_FILENO);
            }
            /* 2. Pipe STDOUT */
            if (i < cp->num_stages - 1) {
                dup2(pipefds[i][1], STDOUT_FILENO);
            }

            /* 3. Close all pipe ends */
            for (int j = 0; j < num_pipes; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            /* 4. Local File Redirections (override pipes if specified) */
            if (s->input_file) {
                int in_fd = open(s->input_file, O_RDONLY);
                if (in_fd < 0) {
                    fprintf(stderr, "myshell: %s: %s\n", s->input_file, strerror(errno));
                    exit(1);
                }
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            if (s->output_file) {
                int flags = O_WRONLY | O_CREAT | (s->append_mode ? O_APPEND : O_TRUNC);
                int out_fd = open(s->output_file, flags, 0644);
                if (out_fd < 0) {
                    fprintf(stderr, "myshell: %s: %s\n", s->output_file, strerror(errno));
                    exit(1);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }

            if (s->err_file) {
                int err_fd = open(s->err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (err_fd < 0) {
                    fprintf(stderr, "myshell: %s: %s\n", s->err_file, strerror(errno));
                    exit(1);
                }
                dup2(err_fd, STDERR_FILENO);
                close(err_fd);
            }

            /* 5. Merged Redirection 2>&1 */
            if (s->merge_err_to_out) {
                dup2(STDOUT_FILENO, STDERR_FILENO);
            }

            /* Execute */
            execvp(s->argv[0], s->argv);
            fprintf(stderr, "myshell: %s: %s\n", s->argv[0], strerror(errno));
            exit(127);
        }
    }

    /* Parent closes all pipe ends */
    for (int j = 0; j < num_pipes; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }
    if (pipefds) free(pipefds);

    /* Wait for all stages */
    int final_status = 0;
    for (int i = 0; i < cp->num_stages; i++) {
        int st;
        waitpid(pids[i], &st, 0);
        if (i == cp->num_stages - 1 && WIFEXITED(st)) {
            final_status = WEXITSTATUS(st);
        }
    }
    free(pids);

    return final_status;
}

void free_complex_pipeline(ComplexPipeline *cp) {
    if (!cp) return;
    for (int i = 0; i < cp->num_stages; i++) {
        ComplexStage *s = &cp->stages[i];
        for (int a = 0; a < s->argc; a++) free(s->argv[a]);
        if (s->argv) free(s->argv);
        if (s->input_file) free(s->input_file);
        if (s->output_file) free(s->output_file);
        if (s->err_file) free(s->err_file);
    }
    if (cp->stages) free(cp->stages);
    free(cp);
}
