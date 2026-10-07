#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "var_expand.h"

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

char *expand_variables(const char *token, int last_status) {
    if (!token) return strdup("");

    char *result = NULL;
    size_t len = 0;
    size_t cap = 0;
    size_t i = 0;
    size_t in_len = strlen(token);

    while (i < in_len) {
        if (token[i] == '$' && i + 1 < in_len) {
            i++; /* skip '$' */
            if (token[i] == '?') {
                char stat_str[16];
                snprintf(stat_str, sizeof(stat_str), "%d", last_status);
                append_str(&result, &len, &cap, stat_str);
                i++;
            } else if (token[i] == '$') {
                char pid_str[16];
                snprintf(pid_str, sizeof(pid_str), "%d", getpid());
                append_str(&result, &len, &cap, pid_str);
                i++;
            } else if (isalpha((unsigned char)token[i]) || token[i] == '_') {
                size_t v_start = i;
                while (i < in_len && (isalnum((unsigned char)token[i]) || token[i] == '_')) {
                    i++;
                }
                size_t v_len = i - v_start;
                char var_name[64];
                if (v_len >= sizeof(var_name)) v_len = sizeof(var_name) - 1;
                strncpy(var_name, token + v_start, v_len);
                var_name[v_len] = '\0';

                const char *val = getenv(var_name);
                /* If variable undefined, expands to empty string */
                if (val) {
                    append_str(&result, &len, &cap, val);
                }
            } else {
                /* Literal '$' if followed by non-variable character */
                append_char(&result, &len, &cap, '$');
                append_char(&result, &len, &cap, token[i]);
                i++;
            }
        } else {
            append_char(&result, &len, &cap, token[i]);
            i++;
        }
    }

    if (!result) return strdup("");
    return result;
}
