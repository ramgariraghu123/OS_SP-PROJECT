#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <signal.h>

#define MAX_LINE 1024
#define MAX_STAGES 16
#define MAX_JOBS 32
#define MAX_HIST 50

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
    char *entries[MAX_HIST];
    int count;
} History;

typedef struct {
    Job jobs[MAX_JOBS];
    int job_count;
    int next_job_id;
    History history;
    int last_status;
    pid_t shell_pgid;
    volatile sig_atomic_t fg_pgid;
    int running;
} Shell;

typedef struct {
    char **argv;
    int argc;
    char *in_file;
    char *out_file;
    int append;
    char *err_file;
} Stage;

typedef struct {
    Stage stages[MAX_STAGES];
    int stage_count;
    int is_background;
    char error_msg[256];
} ParsedCommand;

void shell_init(Shell *sh);
void shell_cleanup(Shell *sh);
void shell_execute(Shell *sh, const char *line);
void shell_check_jobs(Shell *sh);

#endif /* SHELL_H */
