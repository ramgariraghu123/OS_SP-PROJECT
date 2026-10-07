#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "quotes.h"

static void append_char(char **buf, size_t *len, size_t *cap, char c) {
    if (*len + 2 >= *cap) {
        *cap = (*cap == 0) ? 16 : *cap * 2;
        *buf = (char *)realloc(*buf, *cap);
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
}

static void append_str(char **buf, size_t *len, size_t *cap, const char *str) {
    if (!str) return;
    while (*str) {
        append_char(buf, len, cap, *str);
        str++;
    }
}

static void add_token(QuotedTokenList *list, const char *token) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->tokens = (char **)realloc(list->tokens, list->capacity * sizeof(char *));
    }
    list->tokens[list->count++] = strdup(token);
}

QuotedTokenList *parse_with_quotes(const char *input) {
    QuotedTokenList *list = (QuotedTokenList *)calloc(1, sizeof(QuotedTokenList));
    if (!input) return list;

    size_t i = 0;
    size_t in_len = strlen(input);

    char *token_buf = NULL;
    size_t tok_len = 0;
    size_t tok_cap = 0;
    int in_token = 0;

    while (i < in_len) {
        char c = input[i];

        /* Whitespace outside quotes */
        if (isspace((unsigned char)c)) {
            if (in_token) {
                add_token(list, token_buf ? token_buf : "");
                tok_len = 0;
                if (token_buf) token_buf[0] = '\0';
                in_token = 0;
            }
            i++;
            continue;
        }

        in_token = 1;

        if (c == '\'') {
            /* Single Quote: preserve everything verbatim until next single quote */
            i++;
            while (i < in_len && input[i] != '\'') {
                append_char(&token_buf, &tok_len, &tok_cap, input[i]);
                i++;
            }
            if (i >= in_len) {
                snprintf(list->error, sizeof(list->error), "Syntax error: unclosed single quote");
                if (token_buf) free(token_buf);
                return list;
            }
            i++; /* skip closing single quote */
        } else if (c == '"') {
            /* Double Quote: preserve spaces, expand $VARIABLES */
            i++;
            while (i < in_len && input[i] != '"') {
                if (input[i] == '\\' && i + 1 < in_len &&
                    (input[i+1] == '"' || input[i+1] == '\\' || input[i+1] == '$')) {
                    /* Escaped character in double quotes */
                    i++;
                    append_char(&token_buf, &tok_len, &tok_cap, input[i]);
                    i++;
                } else if (input[i] == '$') {
                    /* Variable expansion */
                    i++;
                    if (input[i] == '?') {
                        append_str(&token_buf, &tok_len, &tok_cap, "0");
                        i++;
                    } else if (input[i] == '$') {
                        char pid_str[16];
                        snprintf(pid_str, sizeof(pid_str), "%d", getpid());
                        append_str(&token_buf, &tok_len, &tok_cap, pid_str);
                        i++;
                    } else {
                        size_t v_start = i;
                        while (i < in_len && (isalnum((unsigned char)input[i]) || input[i] == '_')) {
                            i++;
                        }
                        size_t v_len = i - v_start;
                        if (v_len > 0) {
                            char vname[64];
                            if (v_len >= sizeof(vname)) v_len = sizeof(vname) - 1;
                            strncpy(vname, input + v_start, v_len);
                            vname[v_len] = '\0';
                            const char *val = getenv(vname);
                            if (val) append_str(&token_buf, &tok_len, &tok_cap, val);
                        }
                    }
                } else {
                    append_char(&token_buf, &tok_len, &tok_cap, input[i]);
                    i++;
                }
            }
            if (i >= in_len) {
                snprintf(list->error, sizeof(list->error), "Syntax error: unclosed double quote");
                if (token_buf) free(token_buf);
                return list;
            }
            i++; /* skip closing double quote */
        } else if (c == '$') {
            /* Variable expansion outside quotes */
            i++;
            if (input[i] == '?') {
                append_str(&token_buf, &tok_len, &tok_cap, "0");
                i++;
            } else if (input[i] == '$') {
                char pid_str[16];
                snprintf(pid_str, sizeof(pid_str), "%d", getpid());
                append_str(&token_buf, &tok_len, &tok_cap, pid_str);
                i++;
            } else {
                size_t v_start = i;
                while (i < in_len && (isalnum((unsigned char)input[i]) || input[i] == '_')) {
                    i++;
                }
                size_t v_len = i - v_start;
                if (v_len > 0) {
                    char vname[64];
                    if (v_len >= sizeof(vname)) v_len = sizeof(vname) - 1;
                    strncpy(vname, input + v_start, v_len);
                    vname[v_len] = '\0';
                    const char *val = getenv(vname);
                    if (val) append_str(&token_buf, &tok_len, &tok_cap, val);
                }
            }
        } else {
            /* Normal character */
            append_char(&token_buf, &tok_len, &tok_cap, c);
            i++;
        }
    }

    if (in_token) {
        add_token(list, token_buf ? token_buf : "");
    }

    if (token_buf) free(token_buf);
    return list;
}

void free_quoted_token_list(QuotedTokenList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        free(list->tokens[i]);
    }
    if (list->tokens) free(list->tokens);
    free(list);
}
