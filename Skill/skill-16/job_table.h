#ifndef JOB_TABLE_H
#define JOB_TABLE_H

#include <sys/types.h>

#define MAX_JOBS 32

typedef enum {
    JOB_RUNNING,
    JOB_DONE
} JobState;

typedef struct {
    int job_id;
    pid_t pid;
    char *cmd;
    JobState state;
} JobEntry;

typedef struct {
    JobEntry entries[MAX_JOBS];
    int count;
    int next_id;
} JobTable;

JobTable *job_table_init(void);
int job_table_add(JobTable *table, pid_t pid, const char *cmd);
void job_table_check_finished(JobTable *table);
void job_table_display(const JobTable *table);
void job_table_destroy(JobTable *table);

#endif /* JOB_TABLE_H */
