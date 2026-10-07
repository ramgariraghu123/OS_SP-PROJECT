#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "redir_ext.h"

/*
 * OSSP Skill 14: Append Redirection >> and Stderr Redirection 2>.
 */

void dump_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        printf("    | %s", line);
    }
    fclose(f);
}

void run_test(const char *cmdline) {
    printf("------------------------------------------------------------\n");
    printf("Executing: %s\n", cmdline);
    RedirExtCommand *cmd = parse_redir_ext_line(cmdline);

    if (cmd->append_out_file) printf("  [Append Out (>>)]: %s\n", cmd->append_out_file);
    if (cmd->err_file)        printf("  [Stderr (2>)]:     %s\n", cmd->err_file);

    int rc = execute_redir_ext(cmd);
    printf("  Process Exit Status: %d\n", rc);
    free_redir_ext(cmd);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 14: Append Redirection (>>) & Stderr (2>)\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        /* Remove any previous test files */
        remove("test_append.log");
        remove("test_error.log");

        printf("\n[Test 1: First append write to new file]\n");
        run_test("echo [LOG 10:00] System startup initiated >> test_append.log");

        printf("\n[Test 2: Second append write to preserve existing content]\n");
        run_test("echo [LOG 10:01] Modules loaded successfully >> test_append.log");

        printf("\n[Test 3: Third append write]\n");
        run_test("echo [LOG 10:02] Shell ready for commands >> test_append.log");

        printf("\n[Verification: Contents of test_append.log]:\n");
        dump_file("test_append.log");

        printf("\n[Test 4: Stderr redirection 2> to capture error message]\n");
        run_test("ls /invalid_nonexistent_directory_test 2> test_error.log");

        printf("\n[Verification: Contents of test_error.log]:\n");
        dump_file("test_error.log");

        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("append-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        run_test(buffer);
    }

    printf("Exiting Skill 14.\n");
    return 0;
}
