#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include <sys/types.h>

#define MAX_JOBS 16

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobState;

typedef struct {
    int id;
    pid_t pgid;
    char *cmd;
    JobState state;
} SignalJob;

typedef struct {
    SignalJob jobs[MAX_JOBS];
    int count;
    int next_id;
    sigset_t block_mask;
} SignalManager;

SignalManager *sigmgr_init(void);
void sigmgr_block_signals(SignalManager *mgr, sigset_t *prev_mask);
void sigmgr_unblock_signals(const sigset_t *prev_mask);
int sigmgr_add_job(SignalManager *mgr, pid_t pgid, const char *cmd, JobState state);
int sigmgr_bg_resume(SignalManager *mgr, int job_id);
void sigmgr_list(const SignalManager *mgr);
void sigmgr_reap(SignalManager *mgr);
void sigmgr_destroy(SignalManager *mgr);

#endif /* SIGNALS_H */
