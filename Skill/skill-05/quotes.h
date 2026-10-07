#ifndef QUOTES_H
#define QUOTES_H

#include <stddef.h>

typedef struct {
    char **tokens;
    size_t count;
    size_t capacity;
    char error[128];
} QuotedTokenList;

QuotedTokenList *parse_with_quotes(const char *input);
void free_quoted_token_list(QuotedTokenList *list);

#endif /* QUOTES_H */
