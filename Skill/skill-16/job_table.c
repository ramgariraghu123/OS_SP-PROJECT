#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "job_table.h"

JobTable *job_table_init(void) {
    JobTable *t = (JobTable *)calloc(1, sizeof(JobTable));
    t->next_id = 1;
    return t;
}

int job_table_add(JobTable *table, pid_t pid, const char *cmd) {
    if (!table || table->count >= MAX_JOBS) return -1;

    int id = table->next_id++;
    table->entries[table->count].job_id = id;
    table->entries[table->count].pid = pid;
    table->entries[table->count].cmd = strdup(cmd);
    table->entries[table->count].state = JOB_RUNNING;
    table->count++;

    printf("[%d] %d\n", id, pid);
    return id;
}

void job_table_check_finished(JobTable *table) {
    if (!table) return;

    for (int i = 0; i < table->count; ) {
        int status;
        pid_t res = waitpid(table->entries[i].pid, &status, WNOHANG);

        if (res > 0) {
            /* Process has completed */
            printf("[%d]+  Done                    %s\n",
                   table->entries[i].job_id, table->entries[i].cmd);
            free(table->entries[i].cmd);

            /* Shift remaining jobs */
            for (int j = i; j < table->count - 1; j++) {
                table->entries[j] = table->entries[j + 1];
            }
            table->count--;
        } else if (res == -1) {
            /* Process no longer exists */
            free(table->entries[i].cmd);
            for (int j = i; j < table->count - 1; j++) {
                table->entries[j] = table->entries[j + 1];
            }
            table->count--;
        } else {
            /* Still running */
            i++;
        }
    }
}

void job_table_display(const JobTable *table) {
    if (!table || table->count == 0) {
        printf("  [No active background jobs]\n");
        return;
    }

    printf("  JobID   PID      State     Command\n");
    printf("  ------------------------------------\n");
    for (int i = 0; i < table->count; i++) {
        printf("  [%d]     %-7d  Running   %s\n",
               table->entries[i].job_id,
               table->entries[i].pid,
               table->entries[i].cmd);
    }
}

void job_table_destroy(JobTable *table) {
    if (!table) return;
    for (int i = 0; i < table->count; i++) {
        if (table->entries[i].cmd) free(table->entries[i].cmd);
    }
    free(table);
}
