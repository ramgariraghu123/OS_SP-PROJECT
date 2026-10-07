#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "shell.h"

/*
 * OSSP Skill 24: Final Capstone Shell Release & Presentation Suite.
 */

int main(int argc, char *argv[]) {
    Shell sh;
    shell_init(&sh);

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("============================================================\n");
        printf("  OSSP Shell 2.0 (Skill 24 Release) - Quick Test Suite\n");
        printf("============================================================\n");

        shell_execute(&sh, "echo [1/5] Testing Built-ins");
        shell_execute(&sh, "pwd");
        shell_execute(&sh, "export RELEASE_VER=2.0");
        shell_execute(&sh, "echo [2/5] Testing Expansion: Version=$RELEASE_VER, PID=$$");
        shell_execute(&sh, "echo [3/5] Testing Redirection > temp_test.txt");
        shell_execute(&sh, "cat temp_test.txt");
        shell_execute(&sh, "echo [4/5] Testing Multi-stage Pipe");
        shell_execute(&sh, "cat /etc/passwd | cut -d: -f1 | sort | head -n 3");
        shell_execute(&sh, "echo [5/5] Testing Error Recovery");
        shell_execute(&sh, "non_existent_command_demo");

        remove("temp_test.txt");
        shell_cleanup(&sh);
        printf("\nAll release test scenarios completed.\n");
        return 0;
    }

    printf("============================================================\n");
    printf("  OSSP Shell 2.0 - Operating Systems Skill Lab Capstone\n");
    printf("  Author: Vishnu Kurmachalam (2520030199)\n");
    printf("  Type 'help' for built-in documentation, 'exit' to quit.\n");
    printf("============================================================\n");

    char buffer[MAX_LINE];
    while (sh.running) {
        shell_check_jobs(&sh);

        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) {
            char *base = strrchr(cwd, '/');
            if (base && *(base + 1)) base++;
            else base = cwd;
            printf("ossp-shell [%s]> ", base);
        } else {
            printf("ossp-shell> ");
        }
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        if (strlen(buffer) == 0) continue;

        shell_execute(&sh, buffer);
    }

    shell_cleanup(&sh);
    return sh.last_status;
}
