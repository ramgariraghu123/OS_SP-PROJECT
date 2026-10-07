#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "path_resolver.h"

char *resolve_in_path(const char *cmd, char *err_msg, size_t err_len) {
    if (!cmd || strlen(cmd) == 0) {
        if (err_msg) snprintf(err_msg, err_len, "Empty command");
        return NULL;
    }

    /* If command contains a slash, it is an explicit absolute or relative path */
    if (strchr(cmd, '/')) {
        if (access(cmd, F_OK) != 0) {
            if (err_msg) snprintf(err_msg, err_len, "No such file or directory: %s", cmd);
            return NULL;
        }
        if (access(cmd, X_OK) != 0) {
            if (err_msg) snprintf(err_msg, err_len, "Permission denied (not executable): %s", cmd);
            return NULL;
        }
        return strdup(cmd);
    }

    /* Retrieve PATH environment variable */
    const char *path_env = getenv("PATH");
    if (!path_env) {
        path_env = "/bin:/usr/bin:/usr/local/bin";
    }

    char *path_copy = strdup(path_env);
    if (!path_copy) return NULL;

    char *token = strtok(path_copy, ":");
    char full_path[PATH_MAX];

    while (token) {
        snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);
        if (access(full_path, F_OK) == 0) {
            if (access(full_path, X_OK) == 0) {
                free(path_copy);
                return strdup(full_path);
            }
        }
        token = strtok(NULL, ":");
    }

    free(path_copy);
    if (err_msg) snprintf(err_msg, err_len, "Command '%s' not found in PATH", cmd);
    return NULL;
}
