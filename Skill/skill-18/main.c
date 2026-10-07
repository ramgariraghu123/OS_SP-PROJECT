#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include "signals.h"

/*
 * OSSP Skill 18: Signals, Signal Masks, Process Groups,
 * and Resuming Stopped Jobs with bg.
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
    printf("  OSSP Skill 18: Signal Delivery, Masks & 'bg' Resumption\n");
    printf("============================================================\n");

    SignalManager *mgr = sigmgr_init();

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("\n[Test 1: Spawning a child and sending SIGSTOP to simulate Ctrl+Z]\n");
        pid_t pid = fork();
        if (pid == 0) {
            setpgid(0, 0);
            /* Infinite loop until continued and terminated */
            while (1) {
                sleep(1);
            }
            exit(0);
        }
        setpgid(pid, pid);

        /* Stop the child */
        kill(-pid, SIGSTOP);
        int st;
        waitpid(pid, &st, WUNTRACED);

        int jid = sigmgr_add_job(mgr, pid, "sleep 100", JOB_STOPPED);
        printf("  Job [%d] added in Stopped state (PID: %d)\n", jid, pid);

        printf("\n[Test 2: Inspecting job table via 'jobs']\n");
        sigmgr_list(mgr);

        printf("\n[Test 3: Resuming job in background via 'bg 1' (sending SIGCONT)]\n");
        sigmgr_bg_resume(mgr, 1);

        printf("\n[Test 4: Inspecting updated job state]\n");
        sigmgr_list(mgr);

        /* Cleanup child */
        kill(-pid, SIGKILL);
        waitpid(pid, &st, 0);
        sigmgr_destroy(mgr);
        printf("\nSignal tests completed successfully.\n");
        return 0;
    }

    char buffer[1024];
    while (1) {
        sigmgr_reap(mgr);

        printf("sig-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        char *trimmed = trim(buffer);

        if (strcmp(trimmed, "jobs") == 0) {
            sigmgr_list(mgr);
            continue;
        }

        if (strncmp(trimmed, "bg", 2) == 0 && (trimmed[2] == ' ' || trimmed[2] == '\0')) {
            int jid = 1;
            char *arg = trimmed + 2;
            while (*arg == ' ' || *arg == '%') arg++;
            if (*arg != '\0') jid = atoi(arg);
            sigmgr_bg_resume(mgr, jid);
            continue;
        }

        /* Fork external command */
        pid_t p = fork();
        if (p == 0) {
            setpgid(0, 0);
            execlp("/bin/sh", "sh", "-c", trimmed, NULL);
            exit(127);
        }
        setpgid(p, p);
        int st;
        waitpid(p, &st, 0);
    }

    sigmgr_destroy(mgr);
    printf("Exiting Skill 18.\n");
    return 0;
}
