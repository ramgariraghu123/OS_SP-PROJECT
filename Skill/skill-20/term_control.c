#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "term_control.h"

TerminalJobManager *term_mgr_init(void) {
    TerminalJobManager *mgr = (TerminalJobManager *)calloc(1, sizeof(TerminalJobManager));
    mgr->next_id = 1;
    mgr->shell_pgid = getpgrp();

    /* Crucial: ignore job control signals in shell so tcsetpgrp works safely */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    return mgr;
}

int term_mgr_add(TerminalJobManager *mgr, pid_t pgid, const char *cmd, JobStatus st) {
    if (!mgr || mgr->count >= MAX_JOBS) return -1;
    int id = mgr->next_id++;
    mgr->jobs[mgr->count].id = id;
    mgr->jobs[mgr->count].pgid = pgid;
    mgr->jobs[mgr->count].cmd = strdup(cmd);
    mgr->jobs[mgr->count].status = st;
    mgr->count++;
    return id;
}

void term_mgr_list(const TerminalJobManager *mgr) {
    if (!mgr || mgr->count == 0) {
        printf("  [No jobs in table]\n");
        return;
    }
    for (int i = 0; i < mgr->count; i++) {
        printf("[%d]%c  %-10s  %s (PGID: %d)\n",
               mgr->jobs[i].id,
               (i == mgr->count - 1) ? '+' : '-',
               (mgr->jobs[i].status == JOB_STOPPED) ? "Stopped" : "Running",
               mgr->jobs[i].cmd, mgr->jobs[i].pgid);
    }
}

static void remove_job(TerminalJobManager *mgr, int idx) {
    free(mgr->jobs[idx].cmd);
    for (int i = idx; i < mgr->count - 1; i++) {
        mgr->jobs[i] = mgr->jobs[i + 1];
    }
    mgr->count--;
}

int term_mgr_fg(TerminalJobManager *mgr, int id) {
    if (!mgr) return -1;
    int idx = -1;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->jobs[i].id == id) { idx = i; break; }
    }
    if (idx == -1) {
        fprintf(stderr, "fg: %d: no such job\n", id);
        return -1;
    }

    Job *j = &mgr->jobs[idx];
    pid_t pgid = j->pgid;
    printf("%s\n", j->cmd);

    /* Transfer terminal to child group */
    if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, pgid);

    /* Resume if stopped */
    if (j->status == JOB_STOPPED) {
        kill(-pgid, SIGCONT);
        j->status = JOB_RUNNING;
    }

    int status;
    waitpid(pgid, &status, WUNTRACED);

    /* Restore terminal to shell */
    if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, mgr->shell_pgid);

    if (WIFSTOPPED(status)) {
        printf("\n[%d]+ Stopped                 %s\n", j->id, j->cmd);
        j->status = JOB_STOPPED;
    } else {
        remove_job(mgr, idx);
    }
    return 0;
}

int term_mgr_bg(TerminalJobManager *mgr, int id) {
    if (!mgr) return -1;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->jobs[i].id == id) {
            if (mgr->jobs[i].status == JOB_STOPPED) {
                kill(-mgr->jobs[i].pgid, SIGCONT);
                mgr->jobs[i].status = JOB_RUNNING;
                printf("[%d]+ %s &\n", mgr->jobs[i].id, mgr->jobs[i].cmd);
            }
            return 0;
        }
    }
    fprintf(stderr, "bg: %d: no such job\n", id);
    return -1;
}

void term_mgr_reap(TerminalJobManager *mgr) {
    if (!mgr) return;
    for (int i = 0; i < mgr->count; ) {
        int status;
        pid_t res = waitpid(mgr->jobs[i].pgid, &status, WNOHANG | WUNTRACED);
        if (res > 0) {
            if (WIFSTOPPED(status)) {
                mgr->jobs[i].status = JOB_STOPPED;
                i++;
            } else {
                printf("[%d]+  Done                    %s\n",
                       mgr->jobs[i].id, mgr->jobs[i].cmd);
                remove_job(mgr, i);
            }
        } else if (res == -1) {
            remove_job(mgr, i);
        } else {
            i++;
        }
    }
}

void term_mgr_destroy(TerminalJobManager *mgr) {
    if (!mgr) return;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->jobs[i].cmd) free(mgr->jobs[i].cmd);
    }
    free(mgr);
}
