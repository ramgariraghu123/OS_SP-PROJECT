#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "job_control.h"

JobManager *job_mgr_create(void) {
    JobManager *mgr = (JobManager *)calloc(1, sizeof(JobManager));
    mgr->next_id = 1;
    mgr->shell_pgid = getpgrp();
    return mgr;
}

int job_mgr_add(JobManager *mgr, pid_t pgid, const char *cmd, ProcessState state) {
    if (!mgr || mgr->count >= MAX_JOBS) return -1;

    int id = mgr->next_id++;
    mgr->jobs[mgr->count].job_id = id;
    mgr->jobs[mgr->count].pgid = pgid;
    mgr->jobs[mgr->count].cmd = strdup(cmd);
    mgr->jobs[mgr->count].state = state;
    mgr->count++;

    return id;
}

JobRecord *job_mgr_find(JobManager *mgr, int job_id) {
    if (!mgr) return NULL;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->jobs[i].job_id == job_id) {
            return &mgr->jobs[i];
        }
    }
    return NULL;
}

void job_mgr_list(const JobManager *mgr) {
    if (!mgr || mgr->count == 0) {
        printf("  [No active jobs in table]\n");
        return;
    }

    for (int i = 0; i < mgr->count; i++) {
        const char *st_str = (mgr->jobs[i].state == STATE_STOPPED) ? "Stopped" : "Running";
        char marker = (i == mgr->count - 1) ? '+' : '-';
        printf("[%d]%c  %-10s  %s (PGID: %d)\n",
               mgr->jobs[i].job_id, marker, st_str,
               mgr->jobs[i].cmd, mgr->jobs[i].pgid);
    }
}

static void remove_job_at(JobManager *mgr, int index) {
    free(mgr->jobs[index].cmd);
    for (int j = index; j < mgr->count - 1; j++) {
        mgr->jobs[j] = mgr->jobs[j + 1];
    }
    mgr->count--;
}

int job_mgr_bring_to_foreground(JobManager *mgr, int job_id) {
    if (!mgr) return -1;

    int idx = -1;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->jobs[i].job_id == job_id) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        fprintf(stderr, "fg: %d: no such job\n", job_id);
        return -1;
    }

    JobRecord *rec = &mgr->jobs[idx];
    pid_t pgid = rec->pgid;
    printf("%s\n", rec->cmd);

    /* Ignore SIGTTOU so tcsetpgrp does not suspend shell */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    /* Transfer terminal ownership to the job's process group */
    if (isatty(STDIN_FILENO)) {
        tcsetpgrp(STDIN_FILENO, pgid);
    }

    /* If job was stopped, send SIGCONT to resume */
    if (rec->state == STATE_STOPPED) {
        kill(-pgid, SIGCONT);
        rec->state = STATE_RUNNING;
    }

    /* Wait for child process in foreground */
    int status;
    pid_t waited = waitpid(pgid, &status, WUNTRACED);

    /* Restore terminal ownership back to shell */
    if (isatty(STDIN_FILENO)) {
        tcsetpgrp(STDIN_FILENO, mgr->shell_pgid);
    }
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);

    if (waited > 0) {
        if (WIFSTOPPED(status)) {
            printf("\n[%d]+ Stopped                 %s\n", rec->job_id, rec->cmd);
            rec->state = STATE_STOPPED;
        } else {
            /* Finished or killed */
            remove_job_at(mgr, idx);
        }
    }
    return 0;
}

void job_mgr_reap_bg(JobManager *mgr) {
    if (!mgr) return;
    for (int i = 0; i < mgr->count; ) {
        int status;
        pid_t res = waitpid(mgr->jobs[i].pgid, &status, WNOHANG | WUNTRACED);
        if (res > 0) {
            if (WIFSTOPPED(status)) {
                mgr->jobs[i].state = STATE_STOPPED;
                i++;
            } else {
                printf("[%d]+ Done                    %s\n",
                       mgr->jobs[i].job_id, mgr->jobs[i].cmd);
                remove_job_at(mgr, i);
            }
        } else if (res == -1) {
            remove_job_at(mgr, i);
        } else {
            i++;
        }
    }
}

void job_mgr_destroy(JobManager *mgr) {
    if (!mgr) return;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->jobs[i].cmd) free(mgr->jobs[i].cmd);
    }
    free(mgr);
}
