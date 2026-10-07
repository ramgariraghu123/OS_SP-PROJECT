#ifndef ESCAPE_H
#define ESCAPE_H

#include <stddef.h>

typedef struct {
    char **argv;
    int argc;
} ParsedArgs;

ParsedArgs *parse_escaped_line(const char *input);
void free_parsed_args(ParsedArgs *args);

#endif /* ESCAPE_H */
