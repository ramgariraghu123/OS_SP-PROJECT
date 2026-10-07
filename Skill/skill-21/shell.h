#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <signal.h>


#define MAX_LINE_LEN 1024
#define MAX_ARGS 64
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
    char *commands[MAX_HIST];
    int count;
    int next_id;
} ShellHistory;

typedef struct {
    Job jobs[MAX_JOBS];
    int job_count;
    int next_job_id;
    ShellHistory history;
    int last_status;
    pid_t shell_pgid;
    volatile sig_atomic_t fg_pgid;
    int is_running;
} ShellState;

typedef struct {
    char **argv;
    int argc;
    char *input_file;
    char *output_file;
    int append_mode;
    char *err_file;
} Stage;

typedef struct {
    Stage stages[MAX_STAGES];
    int stage_count;
    int is_background;
    char error_msg[256];
} ParsedCommand;

void shell_init(ShellState *sh);
void shell_cleanup(ShellState *sh);
void shell_prompt(ShellState *sh);
void shell_execute_line(ShellState *sh, const char *line);
void shell_check_jobs(ShellState *sh);

#endif /* SHELL_H */
