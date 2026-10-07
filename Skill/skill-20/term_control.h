#ifndef TERM_CONTROL_H
#define TERM_CONTROL_H

#include <sys/types.h>

#define MAX_JOBS 16

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED
} JobStatus;

typedef struct {
    int id;
    pid_t pgid;
    char *cmd;
    JobStatus status;
} Job;

typedef struct {
    Job jobs[MAX_JOBS];
    int count;
    int next_id;
    pid_t shell_pgid;
} TerminalJobManager;

TerminalJobManager *term_mgr_init(void);
int term_mgr_add(TerminalJobManager *mgr, pid_t pgid, const char *cmd, JobStatus st);
void term_mgr_list(const TerminalJobManager *mgr);
int term_mgr_fg(TerminalJobManager *mgr, int id);
int term_mgr_bg(TerminalJobManager *mgr, int id);
void term_mgr_reap(TerminalJobManager *mgr);
void term_mgr_destroy(TerminalJobManager *mgr);

#endif /* TERM_CONTROL_H */
