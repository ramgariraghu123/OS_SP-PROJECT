#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_WORD:            return "WORD";
        case TOKEN_PIPE:            return "PIPE (|)";
        case TOKEN_REDIRECT_IN:     return "REDIR_IN (<)";
        case TOKEN_REDIRECT_OUT:    return "REDIR_OUT (>)";
        case TOKEN_REDIRECT_APPEND: return "REDIR_APPEND (>>)";
        case TOKEN_REDIRECT_ERR:    return "REDIR_ERR (2>)";
        case TOKEN_AMPERSAND:       return "AMPERSAND (&)";
        case TOKEN_EOF:             return "EOF";
        default:                    return "UNKNOWN";
    }
}

static void add_token(TokenStream *stream, TokenType type, const char *val) {
    if (stream->count >= stream->capacity) {
        stream->capacity = stream->capacity == 0 ? 8 : stream->capacity * 2;
        stream->tokens = (Token *)realloc(stream->tokens, stream->capacity * sizeof(Token));
    }
    stream->tokens[stream->count].type = type;
    stream->tokens[stream->count].value = val ? strdup(val) : NULL;
    stream->count++;
}

TokenStream *tokenize(const char *input) {
    TokenStream *stream = (TokenStream *)malloc(sizeof(TokenStream));
    if (!stream) return NULL;
    stream->tokens = NULL;
    stream->count = 0;
    stream->capacity = 0;

    if (!input) return stream;

    size_t i = 0;
    size_t len = strlen(input);

    while (i < len) {
        /* Skip whitespace */
        while (i < len && (input[i] == ' ' || input[i] == '\t' || input[i] == '\n' || input[i] == '\r')) {
            i++;
        }
        if (i >= len) break;

        /* Check multi-character operators */
        if (input[i] == '2' && i + 1 < len && input[i + 1] == '>') {
            add_token(stream, TOKEN_REDIRECT_ERR, "2>");
            i += 2;
            continue;
        }
        if (input[i] == '>' && i + 1 < len && input[i + 1] == '>') {
            add_token(stream, TOKEN_REDIRECT_APPEND, ">>");
            i += 2;
            continue;
        }

        /* Single character operators */
        if (input[i] == '|') {
            add_token(stream, TOKEN_PIPE, "|");
            i++;
            continue;
        }
        if (input[i] == '<') {
            add_token(stream, TOKEN_REDIRECT_IN, "<");
            i++;
            continue;
        }
        if (input[i] == '>') {
            add_token(stream, TOKEN_REDIRECT_OUT, ">");
            i++;
            continue;
        }
        if (input[i] == '&') {
            add_token(stream, TOKEN_AMPERSAND, "&");
            i++;
            continue;
        }

        /* Regular word */
        size_t start = i;
        while (i < len && !isspace((unsigned char)input[i]) &&
               input[i] != '|' && input[i] != '<' && input[i] != '>' && input[i] != '&') {
            i++;
        }
        size_t word_len = i - start;
        char *word = (char *)malloc(word_len + 1);
        strncpy(word, input + start, word_len);
        word[word_len] = '\0';
        add_token(stream, TOKEN_WORD, word);
        free(word);
    }

    add_token(stream, TOKEN_EOF, NULL);
    return stream;
}

void free_token_stream(TokenStream *stream) {
    if (!stream) return;
    for (size_t i = 0; i < stream->count; i++) {
        if (stream->tokens[i].value) {
            free(stream->tokens[i].value);
        }
    }
    if (stream->tokens) free(stream->tokens);
    free(stream);
}
