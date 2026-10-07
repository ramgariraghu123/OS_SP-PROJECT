#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include "shell.h"

void shell_init(Shell *sh) {
    memset(sh, 0, sizeof(Shell));
    sh->is_running = 1;
}

void shell_cleanup(Shell *sh) {
    if (!sh) return;
    for (int i = 0; i < sh->hist_count; i++) {
        if (sh->history[i]) {
            free(sh->history[i]);
            sh->history[i] = NULL;
        }
    }
    sh->hist_count = 0;
}

static void add_history(Shell *sh, const char *cmd) {
    if (!cmd || strlen(cmd) == 0) return;
    if (sh->hist_count < MAX_HIST) {
        sh->history[sh->hist_count++] = strdup(cmd);
    } else {
        free(sh->history[0]);
        for (int i = 0; i < MAX_HIST - 1; i++) {
            sh->history[i] = sh->history[i + 1];
        }
        sh->history[MAX_HIST - 1] = strdup(cmd);
    }
}

static char *expand_token(const char *tok, int last_status) {
    if (!tok) return strdup("");
    char *out = NULL;
    size_t len = 0, cap = 0;
    size_t i = 0, in_len = strlen(tok);

    while (i < in_len) {
        if (tok[i] == '$' && i + 1 < in_len) {
            i++;
            if (tok[i] == '?') {
                char buf[16]; snprintf(buf, sizeof(buf), "%d", last_status);
                for (char *p = buf; *p; p++) {
                    if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; out = realloc(out, cap); }
                    out[len++] = *p;
                }
                i++;
            } else if (tok[i] == '$') {
                char buf[16]; snprintf(buf, sizeof(buf), "%d", getpid());
                for (char *p = buf; *p; p++) {
                    if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; out = realloc(out, cap); }
                    out[len++] = *p;
                }
                i++;
            } else if (isalnum((unsigned char)tok[i]) || tok[i] == '_') {
                size_t start = i;
                while (i < in_len && (isalnum((unsigned char)tok[i]) || tok[i] == '_')) i++;
                char name[64];
                size_t nlen = i - start;
                if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
                strncpy(name, tok + start, nlen);
                name[nlen] = '\0';
                const char *val = getenv(name);
                if (val) {
                    while (*val) {
                        if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; out = realloc(out, cap); }
                        out[len++] = *val++;
                    }
                }
            } else {
                if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; out = realloc(out, cap); }
                out[len++] = '$';
                out[len++] = tok[i++];
            }
        } else {
            if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; out = realloc(out, cap); }
            out[len++] = tok[i++];
        }
    }
    if (len + 1 >= cap) { cap = cap ? cap + 1 : 16; out = realloc(out, cap); }
    if (out) out[len] = '\0';
    return out ? out : strdup("");
}

typedef struct {
    char **argv;
    int argc;
    char *in_file;
    char *out_file;
    int append;
    char *err_file;
} StageCmd;

static void free_stage(StageCmd *s) {
    if (!s) return;
    if (s->argv) {
        for (int i = 0; i < s->argc; i++) free(s->argv[i]);
        free(s->argv);
    }
    if (s->in_file) free(s->in_file);
    if (s->out_file) free(s->out_file);
    if (s->err_file) free(s->err_file);
}

void shell_execute(Shell *sh, const char *line) {
    if (!line) return;
    while (isspace((unsigned char)*line)) line++;
    if (*line == '\0') return;

    add_history(sh, line);

    char *copy = strdup(line);
    size_t clen = strlen(copy);
    while (clen > 0 && isspace((unsigned char)copy[clen - 1])) copy[--clen] = '\0';

    int is_bg = 0;
    if (clen > 0 && copy[clen - 1] == '&') {
        is_bg = 1;
        copy[--clen] = '\0';
        while (clen > 0 && isspace((unsigned char)copy[clen - 1])) copy[--clen] = '\0';
    }

    if (clen > 0 && copy[clen - 1] == '|') {
        fprintf(stderr, "myshell: syntax error near unexpected token '|'\n");
        sh->last_status = 2;
        free(copy);
        return;
    }

    /* Parse stages separated by '|' */
    StageCmd stages[8];
    int num_stages = 0;
    memset(stages, 0, sizeof(stages));

    char *save_pipe;
    char *stage_str = strtok_r(copy, "|", &save_pipe);
    int parse_err = 0;

    while (stage_str && num_stages < 8) {
        StageCmd *st = &stages[num_stages];
        char *save_tok;
        char *tok = strtok_r(stage_str, " \t", &save_tok);
        while (tok) {
            if (strcmp(tok, "<") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (f) st->in_file = strdup(f);
                else parse_err = 1;
            } else if (strcmp(tok, ">") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (f) { st->out_file = strdup(f); st->append = 0; }
                else parse_err = 1;
            } else if (strcmp(tok, ">>") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (f) { st->out_file = strdup(f); st->append = 1; }
                else parse_err = 1;
            } else if (strcmp(tok, "2>") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (f) st->err_file = strdup(f);
                else parse_err = 1;
            } else {
                char *exp = expand_token(tok, sh->last_status);
                st->argv = realloc(st->argv, (st->argc + 2) * sizeof(char *));
                st->argv[st->argc++] = exp;
                st->argv[st->argc] = NULL;
            }
            tok = strtok_r(NULL, " \t", &save_tok);
        }

        if (st->argc == 0) parse_err = 1;
        num_stages++;
        stage_str = strtok_r(NULL, "|", &save_pipe);
    }
    free(copy);

    if (parse_err || num_stages == 0) {
        fprintf(stderr, "myshell: syntax error near unexpected token\n");
        sh->last_status = 2;
        for (int i = 0; i < num_stages; i++) free_stage(&stages[i]);
        return;
    }

    /* Built-ins check if single stage */
    if (num_stages == 1 && !is_bg && !stages[0].in_file && !stages[0].out_file && !stages[0].err_file) {
        char *name = stages[0].argv[0];
        if (strcmp(name, "exit") == 0 || strcmp(name, "quit") == 0) {
            sh->is_running = 0;
            sh->last_status = (stages[0].argc > 1) ? atoi(stages[0].argv[1]) : 0;
            free_stage(&stages[0]);
            return;
        }
        if (strcmp(name, "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
            sh->last_status = 0;
            free_stage(&stages[0]);
            return;
        }
        if (strcmp(name, "cd") == 0) {
            const char *dest = (stages[0].argc > 1) ? stages[0].argv[1] : getenv("HOME");
            if (!dest) dest = "/";
            if (chdir(dest) != 0) {
                fprintf(stderr, "myshell: cd: %s: %s\n", dest, strerror(errno));
                sh->last_status = 1;
            } else {
                sh->last_status = 0;
            }
            free_stage(&stages[0]);
            return;
        }
        if (strcmp(name, "export") == 0) {
            if (stages[0].argc > 1) {
                char *eq = strchr(stages[0].argv[1], '=');
                if (eq) {
                    *eq = '\0';
                    setenv(stages[0].argv[1], eq + 1, 1);
                }
            }
            sh->last_status = 0;
            free_stage(&stages[0]);
            return;
        }
        if (strcmp(name, "echo") == 0) {
            for (int i = 1; i < stages[0].argc; i++) {
                printf("%s%s", stages[0].argv[i], (i + 1 < stages[0].argc) ? " " : "");
            }
            printf("\n");
            sh->last_status = 0;
            free_stage(&stages[0]);
            return;
        }
        if (strcmp(name, "history") == 0) {
            for (int i = 0; i < sh->hist_count; i++) {
                printf("  %2d  %s\n", i + 1, sh->history[i]);
            }
            sh->last_status = 0;
            free_stage(&stages[0]);
            return;
        }
    }

    /* Multi-stage pipeline execution */
    int num_pipes = num_stages - 1;
    int (*pipefds)[2] = NULL;
    if (num_pipes > 0) {
        pipefds = malloc(sizeof(int[2]) * num_pipes);
        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipefds[i]) < 0) {
                perror("pipe");
                free(pipefds);
                for (int s = 0; s < num_stages; s++) free_stage(&stages[s]);
                return;
            }
        }
    }

    pid_t *pids = malloc(sizeof(pid_t) * num_stages);
    for (int i = 0; i < num_stages; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            /* Child */
            if (i > 0) dup2(pipefds[i - 1][0], STDIN_FILENO);
            if (i < num_stages - 1) dup2(pipefds[i][1], STDOUT_FILENO);

            for (int j = 0; j < num_pipes; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            if (stages[i].in_file) {
                int in_fd = open(stages[i].in_file, O_RDONLY);
                if (in_fd < 0) { fprintf(stderr, "myshell: %s: %s\n", stages[i].in_file, strerror(errno)); exit(1); }
                dup2(in_fd, STDIN_FILENO); close(in_fd);
            }
            if (stages[i].out_file) {
                int fl = O_WRONLY | O_CREAT | (stages[i].append ? O_APPEND : O_TRUNC);
                int out_fd = open(stages[i].out_file, fl, 0644);
                if (out_fd < 0) { fprintf(stderr, "myshell: %s: %s\n", stages[i].out_file, strerror(errno)); exit(1); }
                dup2(out_fd, STDOUT_FILENO); close(out_fd);
            }
            if (stages[i].err_file) {
                int err_fd = open(stages[i].err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (err_fd < 0) { fprintf(stderr, "myshell: %s: %s\n", stages[i].err_file, strerror(errno)); exit(1); }
                dup2(err_fd, STDERR_FILENO); close(err_fd);
            }

            execvp(stages[i].argv[0], stages[i].argv);
            if (errno == ENOENT) {
                fprintf(stderr, "myshell: %s: command not found\n", stages[i].argv[0]);
                exit(127);
            }
            exit(1);
        }
    }

    for (int j = 0; j < num_pipes; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }
    if (pipefds) free(pipefds);

    if (is_bg) {
        printf("[bg] started pid %d\n", pids[0]);
    } else {
        for (int i = 0; i < num_stages; i++) {
            int st;
            waitpid(pids[i], &st, 0);
            if (i == num_stages - 1 && WIFEXITED(st)) {
                sh->last_status = WEXITSTATUS(st);
            }
        }
    }

    free(pids);
    for (int i = 0; i < num_stages; i++) free_stage(&stages[i]);
}
