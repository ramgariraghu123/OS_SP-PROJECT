#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include "shell.h"

void stress_shell_init(StressShell *sh) {
    sh->last_status = 0;
    sh->is_running = 1;
}

typedef struct {
    char **argv;
    int argc;
} Stage;

void stress_shell_exec(StressShell *sh, const char *line) {
    if (!line) return;
    while (isspace((unsigned char)*line)) line++;
    if (*line == '\0') return;

    if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
        sh->is_running = 0;
        return;
    }

    char *copy = strdup(line);
    size_t clen = strlen(copy);
    while (clen > 0 && isspace((unsigned char)copy[clen - 1])) copy[--clen] = '\0';

    int is_bg = 0;
    if (clen > 0 && copy[clen - 1] == '&') {
        is_bg = 1;
        copy[--clen] = '\0';
        while (clen > 0 && isspace((unsigned char)copy[clen - 1])) copy[--clen] = '\0';
    }

    Stage stages[16];
    int num_stages = 0;
    memset(stages, 0, sizeof(stages));

    char *save_pipe;
    char *stage_str = strtok_r(copy, "|", &save_pipe);
    while (stage_str && num_stages < 16) {
        char *save_tok;
        char *tok = strtok_r(stage_str, " \t", &save_tok);
        while (tok) {
            stages[num_stages].argv = realloc(stages[num_stages].argv,
                                              (stages[num_stages].argc + 2) * sizeof(char *));
            stages[num_stages].argv[stages[num_stages].argc++] = strdup(tok);
            stages[num_stages].argv[stages[num_stages].argc] = NULL;
            tok = strtok_r(NULL, " \t", &save_tok);
        }
        if (stages[num_stages].argc > 0) num_stages++;
        stage_str = strtok_r(NULL, "|", &save_pipe);
    }
    free(copy);

    if (num_stages == 0) return;

    /* Measure initial resources and start time */
    ResourceSnapshot before = get_resource_snapshot(0);
    double t_start = get_time_sec();

    int num_pipes = num_stages - 1;
    int (*pipefds)[2] = NULL;
    if (num_pipes > 0) {
        pipefds = malloc(sizeof(int[2]) * num_pipes);
        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipefds[i]) < 0) {
                perror("pipe");
                free(pipefds);
                return;
            }
        }
    }

    pid_t *pids = malloc(sizeof(pid_t) * num_stages);
    for (int i = 0; i < num_stages; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            if (i > 0) dup2(pipefds[i - 1][0], STDIN_FILENO);
            if (i < num_stages - 1) dup2(pipefds[i][1], STDOUT_FILENO);
            for (int j = 0; j < num_pipes; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            execvp(stages[i].argv[0], stages[i].argv);
            perror("execvp");
            exit(127);
        }
    }

    for (int j = 0; j < num_pipes; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }
    if (pipefds) free(pipefds);

    if (!is_bg) {
        for (int i = 0; i < num_stages; i++) {
            int st;
            waitpid(pids[i], &st, 0);
            if (i == num_stages - 1 && WIFEXITED(st)) sh->last_status = WEXITSTATUS(st);
        }
        double t_end = get_time_sec();
        ResourceSnapshot after = get_resource_snapshot(0);

        printf("  [Performance Metrics]:\n");
        printf("    Pipeline stages: %d | Latency: %.4f seconds\n", num_stages, t_end - t_start);
        printf("    Memory VmRSS: %ld KB (Delta: %+ld KB) | Open FDs: %d\n",
               after.vm_rss_kb, after.vm_rss_kb - before.vm_rss_kb, after.open_fds);
    } else {
        printf("  [Background job %d launched with %d stages]\n", pids[0], num_stages);
    }

    free(pids);
    for (int i = 0; i < num_stages; i++) {
        for (int a = 0; a < stages[i].argc; a++) free(stages[i].argv[a]);
        free(stages[i].argv);
    }
}
