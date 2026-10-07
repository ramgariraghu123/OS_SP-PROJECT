#ifndef VAR_EXPAND_H
#define VAR_EXPAND_H

/* Expands all $VAR, $?, $$ in a token, returning a dynamically allocated string */
char *expand_variables(const char *token, int last_status);

#endif /* VAR_EXPAND_H */
