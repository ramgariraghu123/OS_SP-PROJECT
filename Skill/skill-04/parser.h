#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"

typedef struct CommandNode {
    char **argv;
    int argc;
    char *input_file;
    char *output_file;
    int append_mode;
    char *err_file;
    struct CommandNode *next;
} CommandNode;

typedef struct {
    CommandNode *commands;
    int command_count;
    int is_background;
    char error_msg[256];
} ParseTree;

ParseTree *parse_tokens(TokenStream *stream);
void print_parse_tree(const ParseTree *tree);
void free_parse_tree(ParseTree *tree);

#endif /* PARSER_H */
