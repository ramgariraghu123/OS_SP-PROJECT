#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quotes.h"

/*
 * OSSP Skill 05: Single Quotes and Double Quotes Handling.
 * Demonstrates:
 *   - Literal preservation in single quotes (no expansion)
 *   - Space preservation and variable expansion in double quotes
 *   - Detection of unclosed quote syntax errors
 */

void test_quote(const char *input) {
    printf("------------------------------------------------------------\n");
    printf("Input Line: %s\n", input);
    QuotedTokenList *list = parse_with_quotes(input);

    if (strlen(list->error) > 0) {
        printf("  [ERROR] %s\n", list->error);
    } else {
        printf("  Parsed %zu token(s):\n", list->count);
        for (size_t i = 0; i < list->count; i++) {
            printf("    Token [%zu]: \"%s\"\n", i, list->tokens[i]);
        }
    }
    free_quoted_token_list(list);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 05: Single and Double Quotes Parser\n");
    printf("============================================================\n");

    /* Ensure environment variables exist for demo */
    setenv("COURSE", "OSSP-25CS2104E", 1);
    setenv("PROJECT", "ShellEngine", 1);

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("[Test 1: Single Quotes preserve literal content, ignore $VAR]\n");
        test_quote("echo 'Hello $USER from $COURSE'");

        printf("\n[Test 2: Double Quotes preserve spaces, expand $VAR]\n");
        test_quote("echo \"Hello $USER from $COURSE\"");

        printf("\n[Test 3: Spaces preserved as single argument]\n");
        test_quote("mkdir \"My Project Documents\"");

        printf("\n[Test 4: Mixed quotes]\n");
        test_quote("echo \"It's a wonderful '$PROJECT'\" 'Nested \"quotes\" demo'");

        printf("\n[Test 5: Edge case - Unclosed single quote]\n");
        test_quote("echo 'unclosed string");

        printf("\n[Test 6: Edge case - Unclosed double quote]\n");
        test_quote("echo \"unclosed string");
        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("quote-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        test_quote(buffer);
    }

    printf("Exiting Skill 05.\n");
    return 0;
}
