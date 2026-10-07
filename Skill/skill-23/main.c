#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"
#include "monitor.h"

/*
 * OSSP Skill 23: Large Pipelines, Multiple Jobs, Stability Testing,
 * and Resource Monitoring.
 */

int main(int argc, char *argv[]) {
    StressShell sh;
    stress_shell_init(&sh);

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("============================================================\n");
        printf("  OSSP Skill 23: Large Pipeline & Stress Performance Demo\n");
        printf("============================================================\n");

        printf("\n--- Test 1: 5-Stage Pipeline with 50,000 Numbers ---\n");
        const char *p1 = "seq 1 50000 | grep 7 | tr 0-9 a-j | sort | head -n 5";
        printf("Executing: %s\n", p1);
        stress_shell_exec(&sh, p1);

        printf("\n--- Test 2: 6-Stage Pipeline with Word Frequency ---\n");
        const char *p2 = "cat /etc/passwd | cut -d: -f7 | sort | uniq -c | sort -n | tail -n 3";
        printf("Executing: %s\n", p2);
        stress_shell_exec(&sh, p2);

        printf("\n--- Test 3: Multiple Concurrent Background Jobs ---\n");
        stress_shell_exec(&sh, "sleep 1 &");
        stress_shell_exec(&sh, "sleep 1 &");
        stress_shell_exec(&sh, "sleep 1 &");
        usleep(1200000);

        ResourceSnapshot snap = get_resource_snapshot(0);
        print_resource_snapshot(&snap, "Final Post-Stress Snapshot");
        return 0;
    }

    char buffer[1024];
    while (sh.is_running) {
        if (isatty(STDIN_FILENO)) {
            printf("stress-shell> ");
            fflush(stdout);
        }

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        if (strlen(buffer) == 0) continue;

        stress_shell_exec(&sh, buffer);
    }

    return 0;
}
