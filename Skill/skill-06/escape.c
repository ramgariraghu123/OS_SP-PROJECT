#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "escape.h"

static void append_char(char **buf, size_t *len, size_t *cap, char c) {
    if (*len + 2 >= *cap) {
        *cap = (*cap == 0) ? 16 : *cap * 2;
        *buf = (char *)realloc(*buf, *cap);
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
}

ParsedArgs *parse_escaped_line(const char *input) {
    ParsedArgs *res = (ParsedArgs *)calloc(1, sizeof(ParsedArgs));
    if (!input) return res;

    size_t in_len = strlen(input);
    size_t i = 0;

    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;
    int in_arg = 0;

    while (i < in_len) {
        char c = input[i];

        if (c == '\\') {
            /* Escape next character */
            in_arg = 1;
            i++;
            if (i < in_len) {
                append_char(&buf, &len, &cap, input[i]);
                i++;
            }
            continue;
        }

        if (isspace((unsigned char)c)) {
            if (in_arg) {
                res->argv = (char **)realloc(res->argv, (res->argc + 2) * sizeof(char *));
                res->argv[res->argc++] = strdup(buf ? buf : "");
                res->argv[res->argc] = NULL;
                len = 0;
                if (buf) buf[0] = '\0';
                in_arg = 0;
            }
            i++;
            continue;
        }

        in_arg = 1;
        append_char(&buf, &len, &cap, c);
        i++;
    }

    if (in_arg) {
        res->argv = (char **)realloc(res->argv, (res->argc + 2) * sizeof(char *));
        res->argv[res->argc++] = strdup(buf ? buf : "");
        res->argv[res->argc] = NULL;
    }

    if (buf) free(buf);
    return res;
}

void free_parsed_args(ParsedArgs *args) {
    if (!args) return;
    if (args->argv) {
        for (int i = 0; i < args->argc; i++) {
            free(args->argv[i]);
        }
        free(args->argv);
    }
    free(args);
}
