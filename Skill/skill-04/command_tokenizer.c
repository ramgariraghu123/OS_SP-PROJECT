/**
 * Skill 04: Command Tokenizer and Parser Logic
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Lexical analysis: converting raw character streams into structured tokens
 * - Delimiter and whitespace processing (spaces, tabs, newlines)
 * - Operator identification (|, <, >, >>, &)
 * - Syntax analysis and error detection (invalid pipes, missing targets)
 * - Parse tree / Execution data structure generation
 * - Complete dynamic memory management (no leaks)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,         // |
    TOKEN_REDIR_IN,     // <
    TOKEN_REDIR_OUT,    // >
    TOKEN_REDIR_APPEND, // >>
    TOKEN_BG,           // &
    TOKEN_EOF
} TokenType;

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_WORD:         return "WORD";
        case TOKEN_PIPE:         return "PIPE (|)";
        case TOKEN_REDIR_IN:     return "REDIR_IN (<)";
        case TOKEN_REDIR_OUT:    return "REDIR_OUT (>)";
        case TOKEN_REDIR_APPEND: return "REDIR_APPEND (>>)";
        case TOKEN_BG:           return "BACKGROUND (&)";
        case TOKEN_EOF:          return "EOF";
        default:                 return "UNKNOWN";
    }
}

typedef struct {
    TokenType type;
    char *value;
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenStream;

TokenStream *token_stream_create() {
    TokenStream *ts = (TokenStream *)malloc(sizeof(TokenStream));
    ts->capacity = 16;
    ts->count = 0;
    ts->items = (Token *)malloc(sizeof(Token) * ts->capacity);
    return ts;
}

void token_stream_add(TokenStream *ts, TokenType type, const char *val) {
    if (ts->count >= ts->capacity) {
        ts->capacity *= 2;
        ts->items = (Token *)realloc(ts->items, sizeof(Token) * ts->capacity);
    }
    ts->items[ts->count].type = type;
    ts->items[ts->count].value = val ? strdup(val) : NULL;
    ts->count++;
}

void token_stream_free(TokenStream *ts) {
    if (!ts) return;
    for (size_t i = 0; i < ts->count; i++) {
        if (ts->items[i].value) free(ts->items[i].value);
    }
    free(ts->items);
    free(ts);
}

// Tokenize input string
TokenStream *tokenize(const char *input) {
    TokenStream *ts = token_stream_create();
    size_t i = 0;
    size_t len = strlen(input);

    while (i < len) {
        while (i < len && isspace((unsigned char)input[i])) i++;
        if (i >= len) break;

        // Check operators
        if (input[i] == '|') {
            token_stream_add(ts, TOKEN_PIPE, "|");
            i++;
        } else if (input[i] == '>') {
            if (i + 1 < len && input[i + 1] == '>') {
                token_stream_add(ts, TOKEN_REDIR_APPEND, ">>");
                i += 2;
            } else {
                token_stream_add(ts, TOKEN_REDIR_OUT, ">");
                i++;
            }
        } else if (input[i] == '<') {
            token_stream_add(ts, TOKEN_REDIR_IN, "<");
            i++;
        } else if (input[i] == '&') {
            token_stream_add(ts, TOKEN_BG, "&");
            i++;
        } else {
            // Word token
            size_t start = i;
            while (i < len && !isspace((unsigned char)input[i]) &&
                   input[i] != '|' && input[i] != '<' && input[i] != '>' && input[i] != '&') {
                i++;
            }
            size_t word_len = i - start;
            char *word = (char *)malloc(word_len + 1);
            strncpy(word, input + start, word_len);
            word[word_len] = '\0';
            token_stream_add(ts, TOKEN_WORD, word);
            free(word);
        }
    }

    token_stream_add(ts, TOKEN_EOF, NULL);
    return ts;
}

// Execution structure for parsed command
typedef struct CommandNode {
    char **argv;
    int argc;
    char *input_file;
    char *output_file;
    bool append_mode;
    bool is_background;
    struct CommandNode *next;
} CommandNode;

CommandNode *command_node_create() {
    CommandNode *node = (CommandNode *)calloc(1, sizeof(CommandNode));
    node->argv = (char **)malloc(sizeof(char *) * 16);
    node->argc = 0;
    return node;
}

void command_node_add_arg(CommandNode *node, const char *arg) {
    node->argv[node->argc] = strdup(arg);
    node->argc++;
    node->argv = (char **)realloc(node->argv, sizeof(char *) * (node->argc + 1));
    node->argv[node->argc] = NULL;
}

void command_tree_free(CommandNode *head) {
    while (head) {
        CommandNode *next = head->next;
        for (int i = 0; i < head->argc; i++) {
            free(head->argv[i]);
        }
        free(head->argv);
        if (head->input_file) free(head->input_file);
        if (head->output_file) free(head->output_file);
        free(head);
        head = next;
    }
}

// Parser that generates CommandNode pipeline and validates syntax
CommandNode *parse_tokens(TokenStream *ts, char *err_msg, size_t err_size) {
    if (ts->count <= 1) return NULL; // Only EOF

    CommandNode *head = command_node_create();
    CommandNode *curr = head;

    for (size_t i = 0; i < ts->count - 1; i++) {
        Token *t = &ts->items[i];

        if (t->type == TOKEN_WORD) {
            command_node_add_arg(curr, t->value);
        } else if (t->type == TOKEN_PIPE) {
            if (curr->argc == 0) {
                snprintf(err_msg, err_size, "Syntax error: pipe '|' with no preceding command");
                command_tree_free(head);
                return NULL;
            }
            if (i + 1 >= ts->count - 1 || ts->items[i + 1].type == TOKEN_PIPE) {
                snprintf(err_msg, err_size, "Syntax error: pipe '|' missing following command");
                command_tree_free(head);
                return NULL;
            }
            CommandNode *next_node = command_node_create();
            curr->next = next_node;
            curr = next_node;
        } else if (t->type == TOKEN_REDIR_IN) {
            if (i + 1 >= ts->count - 1 || ts->items[i + 1].type != TOKEN_WORD) {
                snprintf(err_msg, err_size, "Syntax error: '<' requires input file operand");
                command_tree_free(head);
                return NULL;
            }
            curr->input_file = strdup(ts->items[++i].value);
        } else if (t->type == TOKEN_REDIR_OUT || t->type == TOKEN_REDIR_APPEND) {
            if (i + 1 >= ts->count - 1 || ts->items[i + 1].type != TOKEN_WORD) {
                snprintf(err_msg, err_size, "Syntax error: output redirection requires target file");
                command_tree_free(head);
                return NULL;
            }
            curr->output_file = strdup(ts->items[++i].value);
            curr->append_mode = (t->type == TOKEN_REDIR_APPEND);
        } else if (t->type == TOKEN_BG) {
            curr->is_background = true;
        }
    }

    if (curr->argc == 0) {
        snprintf(err_msg, err_size, "Syntax error: empty command structure");
        command_tree_free(head);
        return NULL;
    }

    return head;
}

void print_parse_tree(CommandNode *head) {
    int idx = 1;
    CommandNode *curr = head;
    printf("\n--- Generated Execution Parse Tree ---\n");
    while (curr) {
        printf("  [Stage %d]:\n", idx++);
        printf("    Executable & Args: ");
        for (int i = 0; i < curr->argc; i++) {
            printf("[%s] ", curr->argv[i]);
        }
        printf("\n");
        printf("    Stdin Redirection : %s\n", curr->input_file ? curr->input_file : "<standard input>");
        printf("    Stdout Redirection: %s (mode: %s)\n",
               curr->output_file ? curr->output_file : "<standard output>",
               curr->append_mode ? "append (>>)" : "truncate (>)");
        printf("    Background Job    : %s\n", curr->is_background ? "YES (&)" : "NO");
        printf("    Has Next Pipe     : %s\n", curr->next ? "YES (|)" : "NO (end of pipeline)");
        curr = curr->next;
    }
    printf("---------------------------------------\n");
}

void test_parsing_case(const char *cmd_line) {
    printf("\nTesting Command: \"%s\"\n", cmd_line);
    TokenStream *ts = tokenize(cmd_line);

    printf("Tokens (%zu):\n  ", ts->count - 1);
    for (size_t i = 0; i < ts->count - 1; i++) {
        printf("[%s: '%s'] ", token_type_name(ts->items[i].type), ts->items[i].value);
    }
    printf("\n");

    char err_msg[128] = {0};
    CommandNode *tree = parse_tokens(ts, err_msg, sizeof(err_msg));

    if (tree) {
        print_parse_tree(tree);
        command_tree_free(tree);
    } else {
        printf("  Validation Result: REJECTED -> %s\n", err_msg);
    }

    token_stream_free(ts);
}

int main() {
    printf("[Skill 04] Command Tokenizer, Lexer, and Syntax Parser\n");

    // Valid cases
    test_parsing_case("ls -l /usr/bin");
    test_parsing_case("cat data.txt | grep error | wc -l > results.log &");
    test_parsing_case("sort < unsorted.txt >> sorted.txt");

    // Syntax error cases
    test_parsing_case("| invalid command");
    test_parsing_case("cat file.txt |");
    test_parsing_case("grep pattern <");

    printf("\n[Skill 04] All tokenizer and parser test cases executed successfully.\n");
    return EXIT_SUCCESS;
}
