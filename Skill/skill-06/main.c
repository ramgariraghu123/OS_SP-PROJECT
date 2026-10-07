#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "escape.h"
#include "executor.h"

/*
 * OSSP Skill 06: Escape Sequences, Child Processes, and Command Launching.
 */

void test_line(const char *cmdline) {
    printf("------------------------------------------------------------\n");
    printf("Input: %s\n", cmdline);
    ParsedArgs *args = parse_escaped_line(cmdline);

    printf("  Parsed %d argument(s):\n", args->argc);
    for (int i = 0; i < args->argc; i++) {
        printf("    argv[%d]: \"%s\"\n", i, args->argv[i]);
    }

    printf("  Executing in child process:\n");
    int rc = execute_command(args);
    printf("  Child Exit Code: %d\n", rc);

    free_parsed_args(args);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 06: Escape Characters & Command Execution\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("[Test 1: Escaped Spaces into a single argument]\n");
        test_line("echo Hello\\ World\\ From\\ OSSP");

        printf("\n[Test 2: Escaped Special Characters]\n");
        test_line("echo Pipe\\|Symbol\\ and\\ Ampersand\\&");

        printf("\n[Test 3: Normal command with multiple arguments]\n");
        test_line("uname -s -r -m");

        printf("\n[Test 4: Error Handling for Non-existent Command]\n");
        test_line("non_existent_command_123");
        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("escape-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        test_line(buffer);
    }

    printf("Exiting Skill 06.\n");
    return 0;
}
