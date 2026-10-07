#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "signals.h"

SignalManager *sigmgr_init(void) {
    SignalManager *mgr = (SignalManager *)calloc(1, sizeof(SignalManager));
    mgr->next_id = 1;

    /* Configure signal mask to protect critical job table sections */
    sigemptyset(&mgr->block_mask);
    sigaddset(&mgr->block_mask, SIGCHLD);
    sigaddset(&mgr->block_mask, SIGINT);
    sigaddset(&mgr->block_mask, SIGTSTP);

    return mgr;
}

void sigmgr_block_signals(SignalManager *mgr, sigset_t *prev_mask) {
    if (sigprocmask(SIG_BLOCK, &mgr->block_mask, prev_mask) < 0) {
        perror("sigprocmask block");
    }
}

void sigmgr_unblock_signals(const sigset_t *prev_mask) {
    if (sigprocmask(SIG_SETMASK, prev_mask, NULL) < 0) {
        perror("sigprocmask unblock");
    }
}

int sigmgr_add_job(SignalManager *mgr, pid_t pgid, const char *cmd, JobState state) {
    sigset_t prev;
    sigmgr_block_signals(mgr, &prev);

    if (mgr->count >= MAX_JOBS) {
        sigmgr_unblock_signals(&prev);
        return -1;
    }

    int id = mgr->next_id++;
    mgr->jobs[mgr->count].id = id;
    mgr->jobs[mgr->count].pgid = pgid;
    mgr->jobs[mgr->count].cmd = strdup(cmd);
    mgr->jobs[mgr->count].state = state;
    mgr->count++;

    sigmgr_unblock_signals(&prev);
    return id;
}

int sigmgr_bg_resume(SignalManager *mgr, int job_id) {
    sigset_t prev;
    sigmgr_block_signals(mgr, &prev);

    SignalJob *target = NULL;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->jobs[i].id == job_id) {
            target = &mgr->jobs[i];
            break;
        }
    }

    if (!target) {
        fprintf(stderr, "bg: %d: no such job\n", job_id);
        sigmgr_unblock_signals(&prev);
        return -1;
    }

    if (target->state != JOB_STOPPED) {
        printf("bg: job %d already running in background\n", job_id);
        sigmgr_unblock_signals(&prev);
        return 0;
    }

    /* Send SIGCONT to the entire process group */
    if (kill(-target->pgid, SIGCONT) < 0) {
        perror("kill SIGCONT");
        sigmgr_unblock_signals(&prev);
        return -1;
    }

    target->state = JOB_RUNNING;
    printf("[%d]+ %s &\n", target->id, target->cmd);

    sigmgr_unblock_signals(&prev);
    return 0;
}

void sigmgr_list(const SignalManager *mgr) {
    if (!mgr || mgr->count == 0) {
        printf("  [No jobs in table]\n");
        return;
    }
    for (int i = 0; i < mgr->count; i++) {
        const char *st = (mgr->jobs[i].state == JOB_STOPPED) ? "Stopped" : "Running";
        printf("[%d]  %-10s  %s (PGID: %d)\n",
               mgr->jobs[i].id, st, mgr->jobs[i].cmd, mgr->jobs[i].pgid);
    }
}

void sigmgr_reap(SignalManager *mgr) {
    sigset_t prev;
    sigmgr_block_signals(mgr, &prev);

    for (int i = 0; i < mgr->count; ) {
        int st;
        pid_t res = waitpid(mgr->jobs[i].pgid, &st, WNOHANG | WUNTRACED);
        if (res > 0) {
            if (WIFSTOPPED(st)) {
                mgr->jobs[i].state = JOB_STOPPED;
                i++;
            } else {
                printf("[%d]+  Done                    %s\n",
                       mgr->jobs[i].id, mgr->jobs[i].cmd);
                free(mgr->jobs[i].cmd);
                for (int j = i; j < mgr->count - 1; j++) {
                    mgr->jobs[j] = mgr->jobs[j + 1];
                }
                mgr->count--;
            }
        } else {
            i++;
        }
    }

    sigmgr_unblock_signals(&prev);
}

void sigmgr_destroy(SignalManager *mgr) {
    if (!mgr) return;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->jobs[i].cmd) free(mgr->jobs[i].cmd);
    }
    free(mgr);
}
