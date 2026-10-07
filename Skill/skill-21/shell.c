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

static ShellState *global_sh = NULL;

static void sigint_handler(int sig) {
    (void)sig;
    if (global_sh && global_sh->fg_pgid > 0) {
        kill(-global_sh->fg_pgid, SIGINT);
    } else {
        const char msg[] = "\n^C\nmyshell-21> ";
        if (write(STDOUT_FILENO, msg, sizeof(msg) - 1) < 0) {}
    }
}

static void sigtstp_handler(int sig) {
    (void)sig;
    if (global_sh && global_sh->fg_pgid > 0) {
        kill(-global_sh->fg_pgid, SIGTSTP);
    }
}

void shell_init(ShellState *sh) {
    memset(sh, 0, sizeof(ShellState));
    sh->next_job_id = 1;
    sh->history.next_id = 1;
    sh->shell_pgid = getpgrp();
    sh->is_running = 1;
    global_sh = sh;

    /* Ignore terminal job control signals in shell */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    /* Setup SIGINT and SIGTSTP handlers */
    struct sigaction sa_int;
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_tstp;
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);
}

void shell_prompt(ShellState *sh) {
    (void)sh;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        char *base = strrchr(cwd, '/');
        if (base && *(base + 1)) base++;
        else base = cwd;
        printf("myshell-21 [%s]> ", base);
    } else {
        printf("myshell-21> ");
    }
    fflush(stdout);
}

static void history_add(ShellState *sh, const char *line) {
    if (!line || strlen(line) == 0) return;
    if (sh->history.count < MAX_HIST) {
        sh->history.commands[sh->history.count++] = strdup(line);
    } else {
        free(sh->history.commands[0]);
        for (int i = 0; i < MAX_HIST - 1; i++) {
            sh->history.commands[i] = sh->history.commands[i + 1];
        }
        sh->history.commands[MAX_HIST - 1] = strdup(line);
    }
    sh->history.next_id++;
}

static int job_add(ShellState *sh, pid_t pgid, const char *cmd, JobStatus st) {
    if (sh->job_count >= MAX_JOBS) return -1;
    int id = sh->next_job_id++;
    sh->jobs[sh->job_count].id = id;
    sh->jobs[sh->job_count].pgid = pgid;
    sh->jobs[sh->job_count].cmd = strdup(cmd);
    sh->jobs[sh->job_count].status = st;
    sh->job_count++;
    return id;
}

static void job_remove(ShellState *sh, int idx) {
    free(sh->jobs[idx].cmd);
    for (int j = idx; j < sh->job_count - 1; j++) {
        sh->jobs[j] = sh->jobs[j + 1];
    }
    sh->job_count--;
}

void shell_check_jobs(ShellState *sh) {
    for (int i = 0; i < sh->job_count; ) {
        int status;
        pid_t res = waitpid(sh->jobs[i].pgid, &status, WNOHANG | WUNTRACED);
        if (res > 0) {
            if (WIFSTOPPED(status)) {
                sh->jobs[i].status = JOB_STOPPED;
                i++;
            } else {
                printf("[%d]+  Done                    %s\n",
                       sh->jobs[i].id, sh->jobs[i].cmd);
                job_remove(sh, i);
            }
        } else if (res == -1 && errno == ECHILD) {
            job_remove(sh, i);
        } else {
            i++;
        }
    }
}

/* Variable expansion and quote processing */
static char *expand_token(const char *in, int last_status) {
    char *out = NULL;
    size_t len = 0, cap = 0;
    size_t i = 0, in_len = strlen(in);

    while (i < in_len) {
        if (in[i] == '$' && i + 1 < in_len) {
            i++;
            if (in[i] == '?') {
                char buf[16]; snprintf(buf, sizeof(buf), "%d", last_status);
                for (char *p = buf; *p; p++) {
                    if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; out = realloc(out, cap); }
                    out[len++] = *p;
                }
                i++;
            } else if (in[i] == '$') {
                char buf[16]; snprintf(buf, sizeof(buf), "%d", getpid());
                for (char *p = buf; *p; p++) {
                    if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; out = realloc(out, cap); }
                    out[len++] = *p;
                }
                i++;
            } else if (isalnum((unsigned char)in[i]) || in[i] == '_') {
                size_t start = i;
                while (i < in_len && (isalnum((unsigned char)in[i]) || in[i] == '_')) i++;
                char name[64];
                size_t nlen = i - start;
                if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
                strncpy(name, in + start, nlen);
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
                out[len++] = in[i++];
            }
        } else {
            if (len + 2 >= cap) { cap = cap ? cap * 2 : 16; out = realloc(out, cap); }
            out[len++] = in[i++];
        }
    }
    if (len + 1 >= cap) { cap = cap ? cap + 1 : 16; out = realloc(out, cap); }
    if (out) out[len] = '\0';
    return out ? out : strdup("");
}

/* Parse line into pipeline stages */
static int parse_command_line(ShellState *sh, const char *line, ParsedCommand *cmd) {
    memset(cmd, 0, sizeof(ParsedCommand));
    char *copy = strdup(line);
    size_t clen = strlen(copy);

    while (clen > 0 && isspace((unsigned char)copy[clen - 1])) copy[--clen] = '\0';
    if (clen > 0 && copy[clen - 1] == '&') {
        cmd->is_background = 1;
        copy[--clen] = '\0';
        while (clen > 0 && isspace((unsigned char)copy[clen - 1])) copy[--clen] = '\0';
    }

    if (clen > 0 && copy[clen - 1] == '|') {
        snprintf(cmd->error_msg, sizeof(cmd->error_msg), "syntax error near unexpected token '|'");
        free(copy);
        return -1;
    }

    char *save_pipe;
    char *stage_str = strtok_r(copy, "|", &save_pipe);
    while (stage_str) {
        if (cmd->stage_count >= MAX_STAGES) {
            snprintf(cmd->error_msg, sizeof(cmd->error_msg), "pipeline stage limit reached");
            free(copy);
            return -1;
        }

        Stage *st = &cmd->stages[cmd->stage_count];
        char *save_tok;
        char *tok = strtok_r(stage_str, " \t", &save_tok);
        while (tok) {
            if (strcmp(tok, "<") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (!f) {
                    snprintf(cmd->error_msg, sizeof(cmd->error_msg), "syntax error: missing file after '<'");
                    free(copy);
                    return -1;
                }
                st->input_file = strdup(f);
            } else if (strcmp(tok, ">") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (!f) {
                    snprintf(cmd->error_msg, sizeof(cmd->error_msg), "syntax error: missing file after '>'");
                    free(copy);
                    return -1;
                }
                st->output_file = strdup(f);
                st->append_mode = 0;
            } else if (strcmp(tok, ">>") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (!f) {
                    snprintf(cmd->error_msg, sizeof(cmd->error_msg), "syntax error: missing file after '>>'");
                    free(copy);
                    return -1;
                }
                st->output_file = strdup(f);
                st->append_mode = 1;
            } else if (strcmp(tok, "2>") == 0) {
                char *f = strtok_r(NULL, " \t", &save_tok);
                if (!f) {
                    snprintf(cmd->error_msg, sizeof(cmd->error_msg), "syntax error: missing file after '2>'");
                    free(copy);
                    return -1;
                }
                st->err_file = strdup(f);
            } else {
                char *exp = expand_token(tok, sh->last_status);
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
        stage_str = strtok_r(NULL, "|", &save_pipe);
    }

    free(copy);
    return 0;
}

static void free_parsed_command(ParsedCommand *cmd) {
    for (int i = 0; i < cmd->stage_count; i++) {
        for (int a = 0; a < cmd->stages[i].argc; a++) free(cmd->stages[i].argv[a]);
        if (cmd->stages[i].argv) free(cmd->stages[i].argv);
        if (cmd->stages[i].input_file) free(cmd->stages[i].input_file);
        if (cmd->stages[i].output_file) free(cmd->stages[i].output_file);
        if (cmd->stages[i].err_file) free(cmd->stages[i].err_file);
    }
}

/* Built-in executions */
static int handle_builtins(ShellState *sh, Stage *st) {
    if (!st || st->argc == 0) return 0;
    char *name = st->argv[0];

    if (strcmp(name, "exit") == 0 || strcmp(name, "quit") == 0) {
        int code = (st->argc > 1) ? atoi(st->argv[1]) : 0;
        sh->is_running = 0;
        sh->last_status = code;
        return 1;
    }

    if (strcmp(name, "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        else perror("pwd");
        sh->last_status = 0;
        return 1;
    }

    if (strcmp(name, "cd") == 0) {
        const char *dest = (st->argc > 1) ? st->argv[1] : getenv("HOME");
        if (!dest) dest = "/";
        if (strcmp(dest, "-") == 0) {
            dest = getenv("OLDPWD");
            if (!dest) {
                fprintf(stderr, "myshell: cd: OLDPWD not set\n");
                sh->last_status = 1;
                return 1;
            }
            printf("%s\n", dest);
        }

        char old[PATH_MAX];
        if (getcwd(old, sizeof(old))) setenv("OLDPWD", old, 1);

        if (chdir(dest) != 0) {
            fprintf(stderr, "myshell: cd: %s: %s\n", dest, strerror(errno));
            sh->last_status = 1;
        } else {
            char cur[PATH_MAX];
            if (getcwd(cur, sizeof(cur))) setenv("PWD", cur, 1);
            sh->last_status = 0;
        }
        return 1;
    }

    if (strcmp(name, "export") == 0) {
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

    if (strcmp(name, "echo") == 0) {
        int newline = 1, start = 1;
        if (st->argc > 1 && strcmp(st->argv[1], "-n") == 0) {
            newline = 0;
            start = 2;
        }
        for (int i = start; i < st->argc; i++) {
            printf("%s%s", st->argv[i], (i + 1 < st->argc) ? " " : "");
        }
        if (newline) printf("\n");
        sh->last_status = 0;
        return 1;
    }

    if (strcmp(name, "history") == 0) {
        for (int i = 0; i < sh->history.count; i++) {
            printf("  %3d  %s\n", i + 1, sh->history.commands[i]);
        }
        sh->last_status = 0;
        return 1;
    }

    if (strcmp(name, "jobs") == 0) {
        if (sh->job_count == 0) printf("  [No active jobs]\n");
        for (int i = 0; i < sh->job_count; i++) {
            const char *st_s = (sh->jobs[i].status == JOB_STOPPED) ? "Stopped" : "Running";
            printf("[%d]%c  %-10s  %s\n", sh->jobs[i].id,
                   (i == sh->job_count - 1) ? '+' : '-', st_s, sh->jobs[i].cmd);
        }
        sh->last_status = 0;
        return 1;
    }

    if (strcmp(name, "fg") == 0) {
        int target_id = (st->argc > 1) ? atoi(st->argv[1]) : (sh->job_count > 0 ? sh->jobs[sh->job_count - 1].id : 0);
        int idx = -1;
        for (int i = 0; i < sh->job_count; i++) {
            if (sh->jobs[i].id == target_id) { idx = i; break; }
        }
        if (idx == -1) {
            fprintf(stderr, "myshell: fg: %d: no such job\n", target_id);
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
            job_remove(sh, idx);
        }
        sh->last_status = 0;
        return 1;
    }

    if (strcmp(name, "bg") == 0) {
        int target_id = (st->argc > 1) ? atoi(st->argv[1]) : (sh->job_count > 0 ? sh->jobs[sh->job_count - 1].id : 0);
        for (int i = 0; i < sh->job_count; i++) {
            if (sh->jobs[i].id == target_id) {
                if (sh->jobs[i].status == JOB_STOPPED) {
                    kill(-sh->jobs[i].pgid, SIGCONT);
                    sh->jobs[i].status = JOB_RUNNING;
                    printf("[%d]+ %s &\n", sh->jobs[i].id, sh->jobs[i].cmd);
                }
                sh->last_status = 0;
                return 1;
            }
        }
        fprintf(stderr, "myshell: bg: %d: no such job\n", target_id);
        sh->last_status = 1;
        return 1;
    }

    if (strcmp(name, "help") == 0) {
        printf("\n=== myshell Built-in Commands ===\n");
        printf("  cd [dir]    : Change working directory (supports ~, -)\n");
        printf("  pwd         : Print current working directory\n");
        printf("  export VAR=val: Set environment variable\n");
        printf("  echo [args] : Print arguments (supports -n)\n");
        printf("  history     : Display command history\n");
        printf("  jobs        : List active background/stopped jobs\n");
        printf("  fg [id]     : Bring job to foreground\n");
        printf("  bg [id]     : Resume stopped job in background\n");
        printf("  help        : Show this help list\n");
        printf("  exit [code] : Exit the shell\n");
        printf("=================================\n\n");
        sh->last_status = 0;
        return 1;
    }

    return 0;
}

void shell_execute_line(ShellState *sh, const char *line) {
    if (!line || strlen(line) == 0) return;

    /* Trim */
    while (isspace((unsigned char)*line)) line++;
    if (strlen(line) == 0) return;

    history_add(sh, line);

    ParsedCommand cmd;
    if (parse_command_line(sh, line, &cmd) < 0) {
        fprintf(stderr, "myshell: %s\n", cmd.error_msg);
        sh->last_status = 2;
        return;
    }

    if (cmd.stage_count == 0) {
        free_parsed_command(&cmd);
        return;
    }

    /* Single built-in optimization */
    if (cmd.stage_count == 1 && !cmd.is_background &&
        !cmd.stages[0].input_file && !cmd.stages[0].output_file && !cmd.stages[0].err_file) {
        if (handle_builtins(sh, &cmd.stages[0])) {
            free_parsed_command(&cmd);
            return;
        }
    }

    /* Pipeline execution */
    int num_pipes = cmd.stage_count - 1;
    int (*pipefds)[2] = NULL;
    if (num_pipes > 0) {
        pipefds = malloc(sizeof(int[2]) * num_pipes);
        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipefds[i]) < 0) {
                perror("myshell: pipe");
                free(pipefds);
                free_parsed_command(&cmd);
                return;
            }
        }
    }

    pid_t *pids = malloc(sizeof(pid_t) * cmd.stage_count);
    pid_t first_pgid = 0;

    for (int i = 0; i < cmd.stage_count; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("myshell: fork");
            break;
        }

        if (pids[i] == 0) {
            /* Child */
            if (i == 0) first_pgid = getpid();
            setpgid(0, first_pgid);

            /* Reset signals in child */
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);

            /* Pipe wiring */
            if (i > 0) dup2(pipefds[i - 1][0], STDIN_FILENO);
            if (i < cmd.stage_count - 1) dup2(pipefds[i][1], STDOUT_FILENO);

            for (int j = 0; j < num_pipes; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            /* File redirections */
            if (cmd.stages[i].input_file) {
                int in_fd = open(cmd.stages[i].input_file, O_RDONLY);
                if (in_fd < 0) {
                    fprintf(stderr, "myshell: %s: %s\n", cmd.stages[i].input_file, strerror(errno));
                    exit(1);
                }
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (cmd.stages[i].output_file) {
                int fl = O_WRONLY | O_CREAT | (cmd.stages[i].append_mode ? O_APPEND : O_TRUNC);
                int out_fd = open(cmd.stages[i].output_file, fl, 0644);
                if (out_fd < 0) {
                    fprintf(stderr, "myshell: %s: %s\n", cmd.stages[i].output_file, strerror(errno));
                    exit(1);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            if (cmd.stages[i].err_file) {
                int err_fd = open(cmd.stages[i].err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (err_fd < 0) {
                    fprintf(stderr, "myshell: %s: %s\n", cmd.stages[i].err_file, strerror(errno));
                    exit(1);
                }
                dup2(err_fd, STDERR_FILENO);
                close(err_fd);
            }

            /* Check built-in in child if inside pipeline */
            if (handle_builtins(sh, &cmd.stages[i])) exit(0);

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
            /* Parent sets pgid */
            if (i == 0) first_pgid = pids[0];
            setpgid(pids[i], first_pgid);
        }
    }

    for (int j = 0; j < num_pipes; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }
    if (pipefds) free(pipefds);

    if (cmd.is_background) {
        int jid = job_add(sh, first_pgid, line, JOB_RUNNING);
        printf("[%d] %d\n", jid, first_pgid);
    } else {
        sh->fg_pgid = first_pgid;
        if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, first_pgid);

        for (int i = 0; i < cmd.stage_count; i++) {
            int st;
            waitpid(pids[i], &st, WUNTRACED);
            if (WIFSTOPPED(st)) {
                job_add(sh, first_pgid, line, JOB_STOPPED);
                printf("\n[%d]+ Stopped                 %s\n", sh->next_job_id - 1, line);
                break;
            }
            if (i == cmd.stage_count - 1 && WIFEXITED(st)) {
                sh->last_status = WEXITSTATUS(st);
            }
        }

        if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, sh->shell_pgid);
        sh->fg_pgid = 0;
    }

    free(pids);
    free_parsed_command(&cmd);
}

void shell_cleanup(ShellState *sh) {
    if (!sh) return;
    for (int i = 0; i < sh->job_count; i++) {
        if (sh->jobs[i].cmd) free(sh->jobs[i].cmd);
    }
    for (int i = 0; i < sh->history.count; i++) {
        if (sh->history.commands[i]) free(sh->history.commands[i]);
    }
}
