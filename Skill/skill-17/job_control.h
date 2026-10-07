#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <sys/types.h>

#define MAX_JOBS 32

typedef enum {
    STATE_RUNNING,
    STATE_STOPPED,
    STATE_DONE
} ProcessState;

typedef struct {
    int job_id;
    pid_t pgid;
    char *cmd;
    ProcessState state;
} JobRecord;

typedef struct {
    JobRecord jobs[MAX_JOBS];
    int count;
    int next_id;
    pid_t shell_pgid;
} JobManager;

JobManager *job_mgr_create(void);
int job_mgr_add(JobManager *mgr, pid_t pgid, const char *cmd, ProcessState state);
JobRecord *job_mgr_find(JobManager *mgr, int job_id);
void job_mgr_list(const JobManager *mgr);
int job_mgr_bring_to_foreground(JobManager *mgr, int job_id);
void job_mgr_reap_bg(JobManager *mgr);
void job_mgr_destroy(JobManager *mgr);

#endif /* JOB_CONTROL_H */
