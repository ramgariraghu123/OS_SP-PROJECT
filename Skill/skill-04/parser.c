#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

static CommandNode *create_command_node(void) {
    CommandNode *node = (CommandNode *)calloc(1, sizeof(CommandNode));
    node->argv = NULL;
    node->argc = 0;
    return node;
}

static void add_argument(CommandNode *node, const char *arg) {
    node->argv = (char **)realloc(node->argv, (node->argc + 2) * sizeof(char *));
    node->argv[node->argc] = strdup(arg);
    node->argc++;
    node->argv[node->argc] = NULL;
}

ParseTree *parse_tokens(TokenStream *stream) {
    ParseTree *tree = (ParseTree *)calloc(1, sizeof(ParseTree));
    if (!stream || stream->count == 0 || stream->tokens[0].type == TOKEN_EOF) {
        return tree; /* Empty command tree */
    }

    size_t i = 0;
    CommandNode *curr_cmd = create_command_node();
    tree->commands = curr_cmd;
    tree->command_count = 1;
    CommandNode *last_cmd = curr_cmd;

    /* Leading pipe check */
    if (stream->tokens[0].type == TOKEN_PIPE) {
        snprintf(tree->error_msg, sizeof(tree->error_msg),
                 "Syntax error: unexpected token '|' at start of command");
        return tree;
    }

    while (i < stream->count && stream->tokens[i].type != TOKEN_EOF) {
        Token *t = &stream->tokens[i];

        if (t->type == TOKEN_WORD) {
            add_argument(curr_cmd, t->value);
            i++;
        } else if (t->type == TOKEN_PIPE) {
            /* Check if current command is empty */
            if (curr_cmd->argc == 0) {
                snprintf(tree->error_msg, sizeof(tree->error_msg),
                         "Syntax error: empty command stage before '|'");
                return tree;
            }
            if (i + 1 < stream->count && stream->tokens[i + 1].type == TOKEN_PIPE) {
                snprintf(tree->error_msg, sizeof(tree->error_msg),
                         "Syntax error: consecutive pipe tokens '||'");
                return tree;
            }
            if (i + 1 < stream->count && stream->tokens[i + 1].type == TOKEN_EOF) {
                snprintf(tree->error_msg, sizeof(tree->error_msg),
                         "Syntax error: unexpected token '|' at end of command");
                return tree;
            }
            /* Create next pipeline stage */
            CommandNode *next_node = create_command_node();
            last_cmd->next = next_node;
            last_cmd = next_node;
            curr_cmd = next_node;
            tree->command_count++;
            i++;
        } else if (t->type == TOKEN_REDIRECT_IN) {
            if (i + 1 >= stream->count || stream->tokens[i + 1].type != TOKEN_WORD) {
                snprintf(tree->error_msg, sizeof(tree->error_msg),
                         "Syntax error: expected filename after '<'");
                return tree;
            }
            curr_cmd->input_file = strdup(stream->tokens[i + 1].value);
            i += 2;
        } else if (t->type == TOKEN_REDIRECT_OUT) {
            if (i + 1 >= stream->count || stream->tokens[i + 1].type != TOKEN_WORD) {
                snprintf(tree->error_msg, sizeof(tree->error_msg),
                         "Syntax error: expected filename after '>'");
                return tree;
            }
            curr_cmd->output_file = strdup(stream->tokens[i + 1].value);
            curr_cmd->append_mode = 0;
            i += 2;
        } else if (t->type == TOKEN_REDIRECT_APPEND) {
            if (i + 1 >= stream->count || stream->tokens[i + 1].type != TOKEN_WORD) {
                snprintf(tree->error_msg, sizeof(tree->error_msg),
                         "Syntax error: expected filename after '>>'");
                return tree;
            }
            curr_cmd->output_file = strdup(stream->tokens[i + 1].value);
            curr_cmd->append_mode = 1;
            i += 2;
        } else if (t->type == TOKEN_REDIRECT_ERR) {
            if (i + 1 >= stream->count || stream->tokens[i + 1].type != TOKEN_WORD) {
                snprintf(tree->error_msg, sizeof(tree->error_msg),
                         "Syntax error: expected filename after '2>'");
                return tree;
            }
            curr_cmd->err_file = strdup(stream->tokens[i + 1].value);
            i += 2;
        } else if (t->type == TOKEN_AMPERSAND) {
            if (i + 1 < stream->count && stream->tokens[i + 1].type != TOKEN_EOF) {
                snprintf(tree->error_msg, sizeof(tree->error_msg),
                         "Syntax error: '&' must be the last token");
                return tree;
            }
            tree->is_background = 1;
            i++;
        } else {
            i++;
        }
    }

    if (curr_cmd->argc == 0 && tree->command_count > 1) {
        snprintf(tree->error_msg, sizeof(tree->error_msg),
                 "Syntax error: empty command at end of pipeline");
    }

    return tree;
}

void print_parse_tree(const ParseTree *tree) {
    if (!tree) return;

    if (strlen(tree->error_msg) > 0) {
        printf("  [PARSER ERROR] %s\n", tree->error_msg);
        return;
    }

    if (tree->command_count == 0 || !tree->commands || tree->commands->argc == 0) {
        printf("  [PARSER INFO] Empty Command.\n");
        return;
    }

    printf("\n=== Parse Tree & Execution Plan ===\n");
    printf("Total Pipeline Stages: %d | Background Mode: %s\n",
           tree->command_count, tree->is_background ? "YES (&)" : "NO");

    int stage = 1;
    CommandNode *curr = tree->commands;
    while (curr) {
        printf("  Stage %d:\n", stage);
        printf("    Command: %s\n", curr->argv && curr->argv[0] ? curr->argv[0] : "(none)");
        printf("    Arguments (%d): [", curr->argc);
        for (int i = 0; i < curr->argc; i++) {
            printf("\"%s\"%s", curr->argv[i], (i + 1 < curr->argc) ? ", " : "");
        }
        printf("]\n");

        if (curr->input_file) {
            printf("    Input Redirection (<): %s\n", curr->input_file);
        }
        if (curr->output_file) {
            printf("    Output Redirection (%s): %s\n",
                   curr->append_mode ? ">>" : ">", curr->output_file);
        }
        if (curr->err_file) {
            printf("    Stderr Redirection (2>): %s\n", curr->err_file);
        }

        curr = curr->next;
        stage++;
    }
    printf("===================================\n\n");
}

void free_parse_tree(ParseTree *tree) {
    if (!tree) return;
    CommandNode *curr = tree->commands;
    while (curr) {
        CommandNode *next = curr->next;
        if (curr->argv) {
            for (int i = 0; i < curr->argc; i++) {
                free(curr->argv[i]);
            }
            free(curr->argv);
        }
        if (curr->input_file) free(curr->input_file);
        if (curr->output_file) free(curr->output_file);
        if (curr->err_file) free(curr->err_file);
        free(curr);
        curr = next;
    }
    free(tree);
}
