#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include "job_control.h"


/*
 * OSSP Skill 17: jobs Command and Foreground Job Switching (fg).
 */

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 17: Job Control, jobs & fg Switching\n");
    printf("============================================================\n");

    JobManager *mgr = job_mgr_create();

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("\n[Test 1: Launch background process 'sleep 1 &']\n");
        pid_t p1 = fork();
        if (p1 == 0) {
            setpgid(0, 0);
            execlp("sleep", "sleep", "1", NULL);
            exit(1);
        }
        setpgid(p1, p1);
        job_mgr_add(mgr, p1, "sleep 1 &", STATE_RUNNING);

        printf("\n[Test 2: Launch simulated stopped process 'sleep 5']\n");
        pid_t p2 = fork();
        if (p2 == 0) {
            setpgid(0, 0);
            execlp("sleep", "sleep", "5", NULL);
            exit(1);
        }
        setpgid(p2, p2);
        job_mgr_add(mgr, p2, "sleep 5", STATE_STOPPED);

        printf("\n[Test 3: Listing active jobs via 'jobs']\n");
        job_mgr_list(mgr);

        printf("\n[Test 4: Bringing Job 1 to foreground via 'fg 1']\n");
        job_mgr_bring_to_foreground(mgr, 1);

        printf("\n[Test 5: Listing jobs after Job 1 completion]\n");
        job_mgr_list(mgr);

        /* Cleanup remaining */
        kill(p2, SIGKILL);
        job_mgr_destroy(mgr);
        return 0;
    }

    char buffer[1024];
    while (1) {
        job_mgr_reap_bg(mgr);

        printf("job-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        char *trimmed = trim(buffer);

        if (strcmp(trimmed, "jobs") == 0) {
            job_mgr_list(mgr);
            continue;
        }

        if (strncmp(trimmed, "fg", 2) == 0 && (trimmed[2] == ' ' || trimmed[2] == '\0')) {
            int jid = 1;
            char *arg = trimmed + 2;
            while (*arg == ' ' || *arg == '%') arg++;
            if (*arg != '\0') jid = atoi(arg);
            job_mgr_bring_to_foreground(mgr, jid);
            continue;
        }

        /* Check if background */
        int is_bg = 0;
        size_t blen = strlen(trimmed);
        if (blen > 0 && trimmed[blen - 1] == '&') {
            is_bg = 1;
            trimmed[blen - 1] = '\0';
            trimmed = trim(trimmed);
        }

        char *argv_cmd[16];
        int c = 0;
        char *tok = strtok(trimmed, " \t");
        while (tok && c < 15) {
            argv_cmd[c++] = tok;
            tok = strtok(NULL, " \t");
        }
        argv_cmd[c] = NULL;
        if (c == 0) continue;

        pid_t pid = fork();
        if (pid == 0) {
            setpgid(0, 0);
            execvp(argv_cmd[0], argv_cmd);
            perror("execvp");
            exit(127);
        }

        setpgid(pid, pid);

        if (is_bg) {
            int id = job_mgr_add(mgr, pid, buffer, STATE_RUNNING);
            printf("[%d] %d\n", id, pid);
        } else {
            /* Foreground */
            signal(SIGTTOU, SIG_IGN);
            if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, pid);
            int st;
            waitpid(pid, &st, WUNTRACED);
            if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, mgr->shell_pgid);
            signal(SIGTTOU, SIG_DFL);
        }
    }

    job_mgr_destroy(mgr);
    printf("Exiting Skill 17.\n");
    return 0;
}
