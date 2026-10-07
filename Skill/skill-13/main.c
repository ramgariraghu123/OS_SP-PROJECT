#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "redir.h"

/*
 * OSSP Skill 13: Input Redirection < and Output Redirection >.
 */

void run_test(const char *cmdline) {
    printf("------------------------------------------------------------\n");
    printf("Executing: %s\n", cmdline);
    RedirCommand *cmd = parse_redir_line(cmdline);

    if (cmd->input_file)  printf("  [Input file]:  %s\n", cmd->input_file);
    if (cmd->output_file) printf("  [Output file]: %s\n", cmd->output_file);

    int rc = execute_with_redirection(cmd);
    printf("  Return code: %d\n", rc);
    free_redir_command(cmd);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 13: Input (<) & Output (>) Redirection\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        /* Prepare sample file */
        FILE *f = fopen("test_input.txt", "w");
        if (f) {
            fprintf(f, "Zebra\nApple\nMango\nBanana\n");
            fclose(f);
        }

        printf("\n[Test 1: Output Redirection (writing to file)]\n");
        run_test("echo Hello OSSP Lab Redirection > test_output.txt");

        printf("\n[Test 2: Input Redirection (reading from file)]\n");
        run_test("cat < test_input.txt");

        printf("\n[Test 3: Both Input and Output Redirection (sorting)]\n");
        run_test("sort < test_input.txt > test_sorted.txt");

        printf("\n[Contents of test_sorted.txt]:\n");
        run_test("cat < test_sorted.txt");

        printf("\n[Test 4: Error handling for missing input file]\n");
        run_test("cat < non_existent_file_404.txt");

        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("redir-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        run_test(buffer);
    }

    printf("Exiting Skill 13.\n");
    return 0;
}
