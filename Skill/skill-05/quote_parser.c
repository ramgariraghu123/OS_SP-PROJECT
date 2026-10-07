/**
 * Skill 05: Quote Handling (Single and Double Quotes)
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Single quote semantics: absolute literal preservation without variable expansion
 * - Double quote semantics: whitespace preservation with environment variable expansion
 * - Finite State Machine (FSM) lexer for state-dependent character interpretation
 * - Edge case handling: unclosed quotes, escaped quotes, adjacent mixed quotes
 * - Memory cleanup with zero leaks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef enum {
    STATE_NORMAL,
    STATE_IN_SINGLE_QUOTE,
    STATE_IN_DOUBLE_QUOTE
} LexerState;

// Buffer helper
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

Buffer *buf_new() {
    Buffer *b = (Buffer *)malloc(sizeof(Buffer));
    b->cap = 32;
    b->len = 0;
    b->data = (char *)malloc(b->cap);
    b->data[0] = '\0';
    return b;
}

void buf_add_char(Buffer *b, char c) {
    if (b->len + 2 > b->cap) {
        b->cap *= 2;
        b->data = (char *)realloc(b->data, b->cap);
    }
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

void buf_add_str(Buffer *b, const char *str) {
    if (!str) return;
    while (*str) {
        buf_add_char(b, *str++);
    }
}

void buf_clear(Buffer *b) {
    b->len = 0;
    b->data[0] = '\0';
}

void buf_free(Buffer *b) {
    if (b) {
        free(b->data);
        free(b);
    }
}

// Token array
typedef struct {
    char **tokens;
    size_t count;
    size_t cap;
} ParsedTokens;

ParsedTokens *parsed_tokens_new() {
    ParsedTokens *pt = (ParsedTokens *)malloc(sizeof(ParsedTokens));
    pt->cap = 8;
    pt->count = 0;
    pt->tokens = (char **)malloc(sizeof(char *) * pt->cap);
    return pt;
}

void parsed_tokens_add(ParsedTokens *pt, const char *word) {
    if (pt->count >= pt->cap) {
        pt->cap *= 2;
        pt->tokens = (char **)realloc(pt->tokens, sizeof(char *) * pt->cap);
    }
    pt->tokens[pt->count++] = strdup(word);
}

void parsed_tokens_free(ParsedTokens *pt) {
    if (!pt) return;
    for (size_t i = 0; i < pt->count; i++) {
        free(pt->tokens[i]);
    }
    free(pt->tokens);
    free(pt);
}

// Helper to expand $VAR
void expand_variable(const char *input, size_t *index, Buffer *out) {
    (*index)++; // skip '$'
    size_t start = *index;
    while (input[*index] && (isalnum((unsigned char)input[*index]) || input[*index] == '_')) {
        (*index)++;
    }
    size_t var_len = *index - start;
    if (var_len == 0) {
        buf_add_char(out, '$');
        return;
    }
    char var_name[128];
    if (var_len >= sizeof(var_name)) var_len = sizeof(var_name) - 1;
    strncpy(var_name, input + start, var_len);
    var_name[var_len] = '\0';

    char *val = getenv(var_name);
    if (val) {
        buf_add_str(out, val);
    }
    // undefined variable expands to empty string (POSIX standard)
}

// Main parser with quote state machine
ParsedTokens *parse_with_quotes(const char *input, char *err_msg, size_t err_size) {
    ParsedTokens *tokens = parsed_tokens_new();
    Buffer *cur_word = buf_new();
    LexerState state = STATE_NORMAL;
    bool in_token = false;
    size_t len = strlen(input);
    size_t i = 0;

    while (i < len) {
        char c = input[i];

        if (state == STATE_NORMAL) {
            if (isspace((unsigned char)c)) {
                if (in_token) {
                    parsed_tokens_add(tokens, cur_word->data);
                    buf_clear(cur_word);
                    in_token = false;
                }
                i++;
            } else if (c == '\'') {
                state = STATE_IN_SINGLE_QUOTE;
                in_token = true;
                i++;
            } else if (c == '"') {
                state = STATE_IN_DOUBLE_QUOTE;
                in_token = true;
                i++;
            } else if (c == '\\') {
                in_token = true;
                i++;
                if (i < len) {
                    buf_add_char(cur_word, input[i++]);
                }
            } else if (c == '$') {
                in_token = true;
                expand_variable(input, &i, cur_word);
            } else {
                in_token = true;
                buf_add_char(cur_word, c);
                i++;
            }
        } else if (state == STATE_IN_SINGLE_QUOTE) {
            if (c == '\'') {
                state = STATE_NORMAL;
                i++;
            } else {
                // Strictly literal: no escape, no expansion
                buf_add_char(cur_word, c);
                i++;
            }
        } else if (state == STATE_IN_DOUBLE_QUOTE) {
            if (c == '"') {
                state = STATE_NORMAL;
                i++;
            } else if (c == '\\') {
                i++;
                if (i < len) {
                    char next_ch = input[i];
                    // Within double quotes, backslash only escapes $, ", \, and newline
                    if (next_ch == '$' || next_ch == '"' || next_ch == '\\' || next_ch == '`') {
                        buf_add_char(cur_word, next_ch);
                        i++;
                    } else {
                        buf_add_char(cur_word, '\\');
                        buf_add_char(cur_word, next_ch);
                        i++;
                    }
                }
            } else if (c == '$') {
                expand_variable(input, &i, cur_word);
            } else {
                buf_add_char(cur_word, c);
                i++;
            }
        }
    }

    if (state == STATE_IN_SINGLE_QUOTE) {
        snprintf(err_msg, err_size, "Syntax error: unmatched single quote (')");
        buf_free(cur_word);
        parsed_tokens_free(tokens);
        return NULL;
    } else if (state == STATE_IN_DOUBLE_QUOTE) {
        snprintf(err_msg, err_size, "Syntax error: unmatched double quote (\")");
        buf_free(cur_word);
        parsed_tokens_free(tokens);
        return NULL;
    }

    if (in_token) {
        parsed_tokens_add(tokens, cur_word->data);
    }

    buf_free(cur_word);
    return tokens;
}

void test_quote_expression(const char *cmd) {
    printf("\nInput command string: [%s]\n", cmd);
    char err[128] = {0};
    ParsedTokens *pt = parse_with_quotes(cmd, err, sizeof(err));

    if (pt) {
        printf("Parsed %zu tokens:\n", pt->count);
        for (size_t i = 0; i < pt->count; i++) {
            printf("  Token [%zu]: \"%s\"\n", i, pt->tokens[i]);
        }
        parsed_tokens_free(pt);
    } else {
        printf("Parsing Failed: %s\n", err);
    }
}

int main() {
    printf("[Skill 05] Single and Double Quote Parser Implementation\n");

    // Set a known environment variable for deterministic testing
    setenv("COURSE", "OSSP-25CS2104E", 1);
    setenv("PROJECT", "UnixShellSuite", 1);

    printf("Environment: COURSE=%s, PROJECT=%s, USER=%s\n",
           getenv("COURSE"), getenv("PROJECT"), getenv("USER") ? getenv("USER") : "student");

    // Case 1: Simple single quotes (literal)
    test_quote_expression("echo 'Hello $COURSE world! Space preserved.'");

    // Case 2: Double quotes (variable expanded and space preserved)
    test_quote_expression("echo \"Welcome to $COURSE - $PROJECT\"");

    // Case 3: Mixed quotes and concatenation
    test_quote_expression("gcc -DNAME=\"My $PROJECT\" -Wall 'file with spaces.c'");

    // Case 4: Escaped quotes inside double quotes
    test_quote_expression("echo \"She said \\\"Hello, $USER\\\"\"");

    // Case 5: Error: unmatched single quote
    test_quote_expression("echo 'Unfinished single quote");

    // Case 6: Error: unmatched double quote
    test_quote_expression("echo \"Unfinished double quote");

    printf("\n[Skill 05] Quote parser tests completed successfully.\n");
    return EXIT_SUCCESS;
}
