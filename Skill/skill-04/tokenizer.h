#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,            /* | */
    TOKEN_REDIRECT_IN,     /* < */
    TOKEN_REDIRECT_OUT,    /* > */
    TOKEN_REDIRECT_APPEND, /* >> */
    TOKEN_REDIRECT_ERR,    /* 2> */
    TOKEN_AMPERSAND,       /* & */
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenStream;

TokenStream *tokenize(const char *input);
void free_token_stream(TokenStream *stream);
const char *token_type_name(TokenType type);

#endif /* TOKENIZER_H */
