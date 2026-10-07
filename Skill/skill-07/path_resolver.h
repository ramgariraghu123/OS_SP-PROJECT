#ifndef PATH_RESOLVER_H
#define PATH_RESOLVER_H

/*
 * Resolves an executable name against the system PATH variable.
 * Returns dynamically allocated full path if found and executable,
 * or NULL if not found or lacking execution permissions.
 */
char *resolve_in_path(const char *cmd, char *err_msg, size_t err_len);

#endif /* PATH_RESOLVER_H */
