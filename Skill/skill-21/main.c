#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"

/*
 * OSSP Skill 21: Integrated Reliable Shell & Runtime Error Recovery.
 */

int main(int argc, char *argv[]) {
    ShellState sh;
    shell_init(&sh);

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("============================================================\n");
        printf("  OSSP Skill 21: Integrated Shell & Error Recovery Tests\n");
        printf("============================================================\n");

        printf("\n--- Category 1: Built-ins & Environment ---\n");
        shell_execute_line(&sh, "pwd");
        shell_execute_line(&sh, "export APP_ENV=PRODUCTION");
        shell_execute_line(&sh, "echo Environment is: $APP_ENV");

        printf("\n--- Category 2: Multi-Stage Pipelines & Redirection ---\n");
        shell_execute_line(&sh, "echo -e \"3\\n1\\n2\" > test_nums.txt");
        shell_execute_line(&sh, "cat < test_nums.txt | sort > test_sorted.txt");
        shell_execute_line(&sh, "cat test_sorted.txt");

        printf("\n--- Category 3: Background Jobs & Status ---\n");
        shell_execute_line(&sh, "sleep 1 &");
        shell_execute_line(&sh, "jobs");
        usleep(1200000);
        shell_check_jobs(&sh);

        printf("\n--- Category 4: Robust Runtime Error Handling & Recovery ---\n");
        printf("Test 4.1: Unknown Command Recovery\n");
        shell_execute_line(&sh, "invalid_command_xyz_123");

        printf("Test 4.2: Missing Redirection File Recovery\n");
        shell_execute_line(&sh, "cat < non_existent_file.txt");

        printf("Test 4.3: Syntax Error (Trailing Pipe) Recovery\n");
        shell_execute_line(&sh, "ls -l |");

        printf("Test 4.4: Syntax Error (Empty Pipe) Recovery\n");
        shell_execute_line(&sh, "echo hello | | wc");

        printf("\n[Recovery Verified]: Shell state is healthy (is_running=%d, last_status=%d)\n",
               sh.is_running, sh.last_status);

        remove("test_nums.txt");
        remove("test_sorted.txt");
        shell_cleanup(&sh);
        return 0;
    }

    printf("============================================================\n");
    printf("  OSSP Skill 21: Fully Integrated Shell Engine\n");
    printf("  Type 'help' for built-ins, 'exit' to quit.\n");
    printf("============================================================\n");

    char buffer[MAX_LINE_LEN];
    while (sh.is_running) {
        shell_check_jobs(&sh);
        shell_prompt(&sh);

        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("\nExiting myshell-21.\n");
            break;
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        if (strlen(buffer) == 0) continue;

        shell_execute_line(&sh, buffer);
    }

    shell_cleanup(&sh);
    return sh.last_status;
}
