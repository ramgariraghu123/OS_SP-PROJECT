#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "var_expand.h"
#include "builtin.h"

/*
 * OSSP Skill 08: Environment-Variable Expansion & Built-in Dispatch Table.
 */

static int last_exit_status = 0;

void process_line(char *line) {
    /* 1. Tokenize into words */
    char *raw_tokens[32];
    int raw_count = 0;
    char *tok = strtok(line, " \t");
    while (tok && raw_count < 31) {
        raw_tokens[raw_count++] = tok;
        tok = strtok(NULL, " \t");
    }
    raw_tokens[raw_count] = NULL;
    if (raw_count == 0) return;

    /* 2. Expand variables for each token */
    char *expanded_argv[32];
    for (int i = 0; i < raw_count; i++) {
        expanded_argv[i] = expand_variables(raw_tokens[i], last_exit_status);
    }
    expanded_argv[raw_count] = NULL;

    printf("  [Tokens after expansion]: ");
    for (int i = 0; i < raw_count; i++) {
        printf("\"%s\" ", expanded_argv[i]);
    }
    printf("\n");

    /* 3. Dispatch through in-process table */
    if (!dispatch_builtin(raw_count, expanded_argv, &last_exit_status)) {
        printf("  [External Command] '%s' is not a shell built-in.\n", expanded_argv[0]);
        last_exit_status = 127;
    }

    /* Cleanup expanded memory */
    for (int i = 0; i < raw_count; i++) {
        free(expanded_argv[i]);
    }
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 08: Variable Expansion & Built-in Dispatch\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        char test1[] = "help";
        printf("\n--- Test 1: Help command via dispatch table ---\n");
        process_line(test1);

        char test2[] = "setvar GREETING WelcomeToOSSP";
        printf("\n--- Test 2: In-process state modification (setvar) ---\n");
        process_line(test2);

        char test3[] = "echo $GREETING User=$USER ShellPID=$$";
        printf("\n--- Test 3: Multiple variable expansion ---\n");
        process_line(test3);

        char test4[] = "echo LastStatus=$? Undefined=$NON_EXISTENT_VAR";
        printf("\n--- Test 4: Special variable $? and undefined variable ---\n");
        process_line(test4);

        char test5[] = "status";
        printf("\n--- Test 5: Internal state display ---\n");
        process_line(test5);

        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("dispatch-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        process_line(buffer);
    }

    printf("Exiting Skill 08.\n");
    return 0;
}
