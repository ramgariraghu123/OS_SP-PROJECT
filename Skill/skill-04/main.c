#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "parser.h"

/*
 * OSSP Skill 04: Tokenization, Delimiters, Whitespace Handling,
 * Token Structures, Parser Logic, Parse Trees, and Syntax Validation.
 */

void test_parse(const char *input) {
    printf("------------------------------------------------------------\n");
    printf("Input: \"%s\"\n", input);

    TokenStream *stream = tokenize(input);
    printf("Token Stream (%zu tokens):\n  ", stream->count);
    for (size_t i = 0; i < stream->count; i++) {
        if (stream->tokens[i].type == TOKEN_EOF) {
            printf("[EOF]\n");
        } else {
            printf("[%s: \"%s\"] ", token_type_name(stream->tokens[i].type),
                   stream->tokens[i].value ? stream->tokens[i].value : "");
        }
    }

    ParseTree *tree = parse_tokens(stream);
    print_parse_tree(tree);

    free_parse_tree(tree);
    free_token_stream(stream);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 04: Shell Tokenizer & Syntax Parse Tree\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        /* Run predefined validation test suite */
        test_parse("ls -l /tmp");
        test_parse("cat < input.txt | grep error | sort > output.txt 2> err.log &");
        test_parse("| invalid_start");
        test_parse("cmd1 | | cmd2");
        test_parse("echo hello >");
        test_parse("sleep 10 & unexpected");
        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("parser> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        test_parse(buffer);
    }

    printf("Exiting Skill 04.\n");
    return 0;
}
