#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include "shell.h"

static Shell *global_shell_ptr = NULL;

static void handle_sigint(int sig) {
    (void)sig;
    if (global_shell_ptr && global_shell_ptr->fg_pgid > 0) {
        kill(-global_shell_ptr->fg_pgid, SIGINT);
    } else {
        const char msg[] = "\n^C\nossp-shell> ";
        if (write(STDOUT_FILENO, msg, sizeof(msg) - 1) < 0) {}
    }
}

static void handle_sigtstp(int sig) {
    (void)sig;
    if (global_shell_ptr && global_shell_ptr->fg_pgid > 0) {
        kill(-global_shell_ptr->fg_pgid, SIGTSTP);
    }
}

void shell_init(Shell *sh) {
    memset(sh, 0, sizeof(Shell));
    sh->next_job_id = 1;
    sh->shell_pgid = getpgrp();
    sh->running = 1;
    global_shell_ptr = sh;

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_tstp;
    sa_tstp.sa_handler = handle_sigtstp;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);
}

void shell_cleanup(Shell *sh) {
    if (!sh) return;
    for (int i = 0; i < sh->job_count; i++) {
        if (sh->jobs[i].cmd) free(sh->jobs[i].cmd);
    }
    for (int i = 0; i < sh->history.count; i++) {
        if (sh->history.entries[i]) free(sh->history.entries[i]);
    }
}

static void add_history(Shell *sh, const char *cmd) {
    if (!cmd || strlen(cmd) == 0) return;
    if (sh->history.count < MAX_HIST) {
        sh->history.entries[sh->history.count++] = strdup(cmd);
    } else {
        free(sh->history.entries[0]);
        for (int i = 0; i < MAX_HIST - 1; i++) sh->history.entries[i] = sh->history.entries[i + 1];
        sh->history.entries[MAX_HIST - 1] = strdup(cmd);
    }
}

static int add_job(Shell *sh, pid_t pgid, const char *cmd, JobStatus st) {
    if (sh->job_count >= MAX_JOBS) return -1;
    int id = sh->next_job_id++;
    sh->jobs[sh->job_count].id = id;
    sh->jobs[sh->job_count].pgid = pgid;
    sh->jobs[sh->job_count].cmd = strdup(cmd);
    sh->jobs[sh->job_count].status = st;
    sh->job_count++;
    return id;
}

static void remove_job(Shell *sh, int idx) {
    free(sh->jobs[idx].cmd);
    for (int j = idx; j < sh->job_count - 1; j++) sh->jobs[j] = sh->jobs[j + 1];
    sh->job_count--;
}

void shell_check_jobs(Shell *sh) {
    for (int i = 0; i < sh->job_count; ) {
        int status;
        pid_t res = waitpid(sh->jobs[i].pgid, &status, WNOHANG | WUNTRACED);
        if (res > 0) {
            if (WIFSTOPPED(status)) {
                sh->jobs[i].status = JOB_STOPPED;
                i++;
            } else {
                printf("[%d]+  Done                    %s\n", sh->jobs[i].id, sh->jobs[i].cmd);
                remove_job(sh, i);
            }
        } else if (res == -1 && errno == ECHILD) {
            remove_job(sh, i);
        } else {
            i++;
        }
    }
}

static char *expand_string(const char *tok, int last_status) {
    if (!tok) return strdup("");
    char *res = NULL;
    size_t len = 0, cap = 0;
    size_t i = 0, tlen = strlen(tok);

    while (i < tlen) {
        if (tok[i] == '$' && i + 1 < tlen) {
            i++;
            if (tok[i] == '?') {
                char buf[16]; snprintf(buf, sizeof(buf), "%d", last_status);
                for (char *p = buf; *p; p++) {
                    if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; res = realloc(res, cap); }
                    res[len++] = *p;
                }
                i++;
            } else if (tok[i] == '$') {
                char buf[16]; snprintf(buf, sizeof(buf), "%d", getpid());
                for (char *p = buf; *p; p++) {
                    if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; res = realloc(res, cap); }
                    res[len++] = *p;
                }
                i++;
            } else if (isalnum((unsigned char)tok[i]) || tok[i] == '_') {
                size_t s = i;
                while (i < tlen && (isalnum((unsigned char)tok[i]) || tok[i] == '_')) i++;
                char name[64];
                size_t nlen = i - s;
                if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
                strncpy(name, tok + s, nlen);
                name[nlen] = '\0';
                const char *v = getenv(name);
                if (v) {
                    while (*v) {
                        if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; res = realloc(res, cap); }
                        res[len++] = *v++;
                    }
                }
            } else {
                if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; res = realloc(res, cap); }
                res[len++] = '$'; res[len++] = tok[i++];
            }
        } else {
            if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; res = realloc(res, cap); }
            res[len++] = tok[i++];
        }
    }
    if (len + 1 >= cap) { cap = cap ? cap + 1 : 16; res = realloc(res, cap); }
    if (res) res[len] = '\0';
    return res ? res : strdup("");
}

static void free_cmd(ParsedCommand *cmd) {
    for (int i = 0; i < cmd->stage_count; i++) {
        for (int a = 0; a < cmd->stages[i].argc; a++) free(cmd->stages[i].argv[a]);
        if (cmd->stages[i].argv) free(cmd->stages[i].argv);
        if (cmd->stages[i].in_file) free(cmd->stages[i].in_file);
        if (cmd->stages[i].out_file) free(cmd->stages[i].out_file);
        if (cmd->stages[i].err_file) free(cmd->stages[i].err_file);
    }
}

static int parse_line(Shell *sh, const char *line, ParsedCommand *cmd) {
    memset(cmd, 0, sizeof(ParsedCommand));
    char *copy = strdup(line);
    size_t len = strlen(copy);

    while (len > 0 && isspace((unsigned char)copy[len - 1])) copy[--len] = '\0';
    if (len > 0 && copy[len - 1] == '&') {
        cmd->is_background = 1;
        copy[--len] = '\0';
        while (len > 0 && isspace((unsigned char)copy[len - 1])) copy[--len] = '\0';
    }

    if (len > 0 && copy[len - 1] == '|') {
        snprintf(cmd->error_msg, sizeof(cmd->error_msg), "syntax error near unexpected token '|'");
        free(copy);
        return -1;
    }

    char *save_pipe;
    char *s_str = strtok_r(copy, "|", &save_pipe);
    while (s_str) {
        if (cmd->stage_count >= MAX_STAGES) {
            snprintf(cmd->error_msg, sizeof(cmd->error_msg), "stage limit reached");
            free(copy);
            return -1;
        }

        Stage *st = &cmd->stages[cmd->stage_count];
        char *save_tok;
        char *tok = strtok_r(s_str, " \t", &save_tok);
        while (tok) {
            if (strcmp(tok, "<") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (f) st->in_file = strdup(f);
            } else if (strcmp(tok, ">") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (f) { st->out_file = strdup(f); st->append = 0; }
            } else if (strcmp(tok, ">>") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (f) { st->out_file = strdup(f); st->append = 1; }
            } else if (strcmp(tok, "2>") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (f) st->err_file = strdup(f);
            } else {
                char *exp = expand_string(tok, sh->last_status);
                st->argv = realloc(st->argv, (st->argc + 2) * sizeof(char *));
                st->argv[st->argc++] = exp;
                st->argv[st->argc] = NULL;
            }
            tok = strtok_r(NULL, " \t", &save_tok);
        }

        if (st->argc == 0) {
            snprintf(cmd->error_msg, sizeof(cmd->error_msg), "syntax error near unexpected token '|'");
            free(copy);
            return -1;
        }

        cmd->stage_count++;
        s_str = strtok_r(NULL, "|", &save_pipe);
    }
    free(copy);
    return 0;
}

static int run_builtin(Shell *sh, Stage *st) {
    if (!st || st->argc == 0) return 0;
    char *cmd = st->argv[0];

    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        sh->running = 0;
        sh->last_status = (st->argc > 1) ? atoi(st->argv[1]) : 0;
        return 1;
    }
    if (strcmp(cmd, "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        sh->last_status = 0;
        return 1;
    }
    if (strcmp(cmd, "cd") == 0) {
        const char *tgt = (st->argc > 1) ? st->argv[1] : getenv("HOME");
        if (!tgt) tgt = "/";
        if (strcmp(tgt, "-") == 0) {
            tgt = getenv("OLDPWD");
            if (!tgt) { fprintf(stderr, "cd: OLDPWD not set\n"); sh->last_status = 1; return 1; }
            printf("%s\n", tgt);
        }
        char old[PATH_MAX];
        if (getcwd(old, sizeof(old))) setenv("OLDPWD", old, 1);
        if (chdir(tgt) != 0) {
            fprintf(stderr, "cd: %s: %s\n", tgt, strerror(errno));
            sh->last_status = 1;
        } else {
            char cur[PATH_MAX];
            if (getcwd(cur, sizeof(cur))) setenv("PWD", cur, 1);
            sh->last_status = 0;
        }
        return 1;
    }
    if (strcmp(cmd, "export") == 0) {
        if (st->argc < 2) {
            extern char **environ;
            for (char **e = environ; *e; e++) printf("declare -x %s\n", *e);
            sh->last_status = 0;
            return 1;
        }
        for (int i = 1; i < st->argc; i++) {
            char *eq = strchr(st->argv[i], '=');
            if (eq) {
                *eq = '\0';
                setenv(st->argv[i], eq + 1, 1);
                *eq = '=';
            } else {
                setenv(st->argv[i], "", 1);
            }
        }
        sh->last_status = 0;
        return 1;
    }
    if (strcmp(cmd, "echo") == 0) {
        int nl = 1, start = 1;
        if (st->argc > 1 && strcmp(st->argv[1], "-n") == 0) { nl = 0; start = 2; }
        for (int i = start; i < st->argc; i++) printf("%s%s", st->argv[i], (i + 1 < st->argc) ? " " : "");
        if (nl) printf("\n");
        sh->last_status = 0;
        return 1;
    }
    if (strcmp(cmd, "history") == 0) {
        for (int i = 0; i < sh->history.count; i++) printf("  %3d  %s\n", i + 1, sh->history.entries[i]);
        sh->last_status = 0;
        return 1;
    }
    if (strcmp(cmd, "jobs") == 0) {
        if (sh->job_count == 0) printf("  [No active jobs]\n");
        for (int i = 0; i < sh->job_count; i++) {
            printf("[%d]%c  %-10s  %s\n", sh->jobs[i].id,
                   (i == sh->job_count - 1) ? '+' : '-',
                   (sh->jobs[i].status == JOB_STOPPED) ? "Stopped" : "Running",
                   sh->jobs[i].cmd);
        }
        sh->last_status = 0;
        return 1;
    }
    if (strcmp(cmd, "fg") == 0) {
        int target = (st->argc > 1) ? atoi(st->argv[1]) : (sh->job_count > 0 ? sh->jobs[sh->job_count - 1].id : 0);
        int idx = -1;
        for (int i = 0; i < sh->job_count; i++) {
            if (sh->jobs[i].id == target) { idx = i; break; }
        }
        if (idx == -1) {
            fprintf(stderr, "fg: %d: no such job\n", target);
            sh->last_status = 1;
            return 1;
        }

        pid_t pgid = sh->jobs[idx].pgid;
        printf("%s\n", sh->jobs[idx].cmd);
        sh->fg_pgid = pgid;
        if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, pgid);
        if (sh->jobs[idx].status == JOB_STOPPED) kill(-pgid, SIGCONT);

        int status;
        waitpid(pgid, &status, WUNTRACED);
        if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, sh->shell_pgid);
        sh->fg_pgid = 0;

        if (WIFSTOPPED(status)) {
            sh->jobs[idx].status = JOB_STOPPED;
            printf("\n[%d]+ Stopped                 %s\n", sh->jobs[idx].id, sh->jobs[idx].cmd);
        } else {
            remove_job(sh, idx);
        }
        sh->last_status = 0;
        return 1;
    }
    if (strcmp(cmd, "bg") == 0) {
        int target = (st->argc > 1) ? atoi(st->argv[1]) : (sh->job_count > 0 ? sh->jobs[sh->job_count - 1].id : 0);
        for (int i = 0; i < sh->job_count; i++) {
            if (sh->jobs[i].id == target) {
                if (sh->jobs[i].status == JOB_STOPPED) {
                    kill(-sh->jobs[i].pgid, SIGCONT);
                    sh->jobs[i].status = JOB_RUNNING;
                    printf("[%d]+ %s &\n", sh->jobs[i].id, sh->jobs[i].cmd);
                }
                sh->last_status = 0;
                return 1;
            }
        }
        fprintf(stderr, "bg: %d: no such job\n", target);
        sh->last_status = 1;
        return 1;
    }
    if (strcmp(cmd, "help") == 0) {
        printf("\n============================================================\n");
        printf("  OSSP Shell 2.0 (Final Release) - Built-in Commands\n");
        printf("============================================================\n");
        printf("  cd [dir]       : Change directory (supports ~, -)\n");
        printf("  pwd            : Display current working directory\n");
        printf("  export VAR=val : Export environment variable\n");
        printf("  echo [args]    : Display args (-n to suppress newline)\n");
        printf("  history        : Display command history log\n");
        printf("  jobs           : List background and stopped jobs\n");
        printf("  fg [id]        : Switch job to foreground\n");
        printf("  bg [id]        : Resume stopped job in background\n");
        printf("  help           : Display available commands\n");
        printf("  exit [code]    : Exit the shell session\n");
        printf("============================================================\n\n");
        sh->last_status = 0;
        return 1;
    }
    return 0;
}

void shell_execute(Shell *sh, const char *line) {
    if (!line || strlen(line) == 0) return;
    while (isspace((unsigned char)*line)) line++;
    if (*line == '\0') return;

    add_history(sh, line);

    ParsedCommand cmd;
    if (parse_line(sh, line, &cmd) < 0) {
        fprintf(stderr, "myshell: %s\n", cmd.error_msg);
        sh->last_status = 2;
        return;
    }
    if (cmd.stage_count == 0) { free_cmd(&cmd); return; }

    /* Single built-in */
    if (cmd.stage_count == 1 && !cmd.is_background &&
        !cmd.stages[0].in_file && !cmd.stages[0].out_file && !cmd.stages[0].err_file) {
        if (run_builtin(sh, &cmd.stages[0])) {
            free_cmd(&cmd);
            return;
        }
    }

    int num_pipes = cmd.stage_count - 1;
    int (*pipefds)[2] = NULL;
    if (num_pipes > 0) {
        pipefds = malloc(sizeof(int[2]) * num_pipes);
        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipefds[i]) < 0) {
                perror("pipe");
                free(pipefds);
                free_cmd(&cmd);
                return;
            }
        }
    }

    pid_t *pids = malloc(sizeof(pid_t) * cmd.stage_count);
    pid_t lead_pgid = 0;

    for (int i = 0; i < cmd.stage_count; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            /* Child */
            if (i == 0) lead_pgid = getpid();
            setpgid(0, lead_pgid);

            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);

            if (i > 0) dup2(pipefds[i - 1][0], STDIN_FILENO);
            if (i < cmd.stage_count - 1) dup2(pipefds[i][1], STDOUT_FILENO);

            for (int j = 0; j < num_pipes; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            if (cmd.stages[i].in_file) {
                int in_fd = open(cmd.stages[i].in_file, O_RDONLY);
                if (in_fd < 0) { fprintf(stderr, "myshell: %s: %s\n", cmd.stages[i].in_file, strerror(errno)); exit(1); }
                dup2(in_fd, STDIN_FILENO); close(in_fd);
            }
            if (cmd.stages[i].out_file) {
                int fl = O_WRONLY | O_CREAT | (cmd.stages[i].append ? O_APPEND : O_TRUNC);
                int out_fd = open(cmd.stages[i].out_file, fl, 0644);
                if (out_fd < 0) { fprintf(stderr, "myshell: %s: %s\n", cmd.stages[i].out_file, strerror(errno)); exit(1); }
                dup2(out_fd, STDOUT_FILENO); close(out_fd);
            }
            if (cmd.stages[i].err_file) {
                int err_fd = open(cmd.stages[i].err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (err_fd < 0) { fprintf(stderr, "myshell: %s: %s\n", cmd.stages[i].err_file, strerror(errno)); exit(1); }
                dup2(err_fd, STDERR_FILENO); close(err_fd);
            }

            if (run_builtin(sh, &cmd.stages[i])) exit(0);

            execvp(cmd.stages[i].argv[0], cmd.stages[i].argv);
            if (errno == ENOENT) {
                fprintf(stderr, "myshell: command not found: %s\n", cmd.stages[i].argv[0]);
                exit(127);
            } else if (errno == EACCES) {
                fprintf(stderr, "myshell: %s: Permission denied\n", cmd.stages[i].argv[0]);
                exit(126);
            } else {
                fprintf(stderr, "myshell: %s: %s\n", cmd.stages[i].argv[0], strerror(errno));
                exit(1);
            }
        } else {
            if (i == 0) lead_pgid = pids[0];
            setpgid(pids[i], lead_pgid);
        }
    }

    for (int j = 0; j < num_pipes; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }
    if (pipefds) free(pipefds);

    if (cmd.is_background) {
        int id = add_job(sh, lead_pgid, line, JOB_RUNNING);
        printf("[%d] %d\n", id, lead_pgid);
    } else {
        sh->fg_pgid = lead_pgid;
        if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, lead_pgid);

        for (int i = 0; i < cmd.stage_count; i++) {
            int st;
            waitpid(pids[i], &st, WUNTRACED);
            if (WIFSTOPPED(st)) {
                add_job(sh, lead_pgid, line, JOB_STOPPED);
                printf("\n[%d]+ Stopped                 %s\n", sh->next_job_id - 1, line);
                break;
            }
            if (i == cmd.stage_count - 1 && WIFEXITED(st)) sh->last_status = WEXITSTATUS(st);
        }

        if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, sh->shell_pgid);
        sh->fg_pgid = 0;
    }

    free(pids);
    free_cmd(&cmd);
}
