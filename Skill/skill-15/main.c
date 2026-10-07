#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "complex_exec.h"

/*
 * OSSP Skill 15: Combined Redirection & Complex Pipeline-Redirection Hybrids.
 */

void dump_file(const char *name) {
    FILE *f = fopen(name, "r");
    if (!f) return;
    printf("  [File %s contents]:\n", name);
    char buf[256];
    while (fgets(buf, sizeof(buf), f)) {
        printf("    | %s", buf);
    }
    fclose(f);
}

void test_complex_command(const char *cmdline) {
    printf("------------------------------------------------------------\n");
    printf("Input: %s\n", cmdline);
    ComplexPipeline *cp = parse_complex_command(cmdline);
    print_execution_plan(cp);
    int rc = execute_complex_pipeline(cp);
    printf("Pipeline Execution Completed with status: %d\n", rc);
    free_complex_pipeline(cp);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 15: Complex Combined Redirection & Pipelines\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        /* Prepare input test file */
        FILE *f = fopen("complex_in.txt", "w");
        if (f) {
            fprintf(f, "delta\nalpha\ncharlie\nbravo\n");
            fclose(f);
        }

        printf("\n[Test 1: Combined Input + Multi-Pipe + Output + Stderr Redirection]\n");
        test_complex_command("cat < complex_in.txt | tr a-z A-Z | sort > complex_out.txt 2> complex_err.log");
        dump_file("complex_out.txt");

        printf("\n[Test 2: Merged Streams (2>&1)]\n");
        test_complex_command("ls /valid_nonexistent_test 2>&1 > merged.log");
        test_complex_command("ls /valid_nonexistent_test > merged.log 2>&1");
        dump_file("merged.log");

        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("complex-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        test_complex_command(buffer);
    }

    printf("Exiting Skill 15.\n");
    return 0;
}
