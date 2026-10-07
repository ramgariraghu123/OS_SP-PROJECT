#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include "job_table.h"

/*
 * OSSP Skill 16: Background Jobs (&), Non-blocking Processes,
 * Job Table, and WNOHANG Monitoring.
 */

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

void execute_bg_cmd(JobTable *table, const char *input) {
    char *copy = strdup(input);
    char *trimmed = trim(copy);

    int is_bg = 0;
    size_t len = strlen(trimmed);
    if (len > 0 && trimmed[len - 1] == '&') {
        is_bg = 1;
        trimmed[len - 1] = '\0';
        trimmed = trim(trimmed);
    }

    char *argv[16];
    int argc = 0;
    char *tok = strtok(trimmed, " \t");
    while (tok && argc < 15) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    argv[argc] = NULL;

    if (argc == 0) {
        free(copy);
        return;
    }

    if (strcmp(argv[0], "jobs") == 0) {
        job_table_display(table);
        free(copy);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(copy);
        return;
    }

    if (pid == 0) {
        /* Child process: assign own process group */
        setpgid(0, 0);
        execvp(argv[0], argv);
        perror("execvp");
        exit(127);
    }

    if (is_bg) {
        /* Background job: add to table and return prompt immediately */
        job_table_add(table, pid, input);
    } else {
        /* Foreground job: wait synchronously */
        int status;
        waitpid(pid, &status, 0);
    }

    free(copy);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 16: Background Jobs (&) & Non-blocking Table\n");
    printf("============================================================\n");

    JobTable *table = job_table_init();

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("\n[Test 1: Launch background job (sleep 1 &)]\n");
        execute_bg_cmd(table, "sleep 1 &");

        printf("\n[Test 2: Launch second background job (sleep 2 &)]\n");
        execute_bg_cmd(table, "sleep 2 &");

        printf("\n[Test 3: List running jobs immediately]\n");
        job_table_display(table);

        printf("\n[Test 4: Sleep for 1.2 seconds in main thread to allow job 1 to complete]\n");
        usleep(1200000);
        printf("[Reaping background jobs via WNOHANG]:\n");
        job_table_check_finished(table);

        printf("\n[Test 5: List remaining jobs]\n");
        job_table_display(table);

        printf("\n[Test 6: Sleep remaining time and reap job 2]:\n");
        usleep(1200000);
        job_table_check_finished(table);
        job_table_display(table);

        job_table_destroy(table);
        return 0;
    }

    char buffer[1024];
    while (1) {
        /* Check finished background jobs before displaying prompt */
        job_table_check_finished(table);

        printf("bg-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        execute_bg_cmd(table, buffer);
    }

    job_table_destroy(table);
    printf("Exiting Skill 16.\n");
    return 0;
}
