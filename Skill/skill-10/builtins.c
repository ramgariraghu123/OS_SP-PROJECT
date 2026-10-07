#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include "builtins.h"


int builtin_pwd(int argc, char **argv) {
    (void)argc;
    (void)argv;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("%s\n", cwd);
        return 0;
    }
    perror("pwd");
    return 1;
}

static int is_valid_identifier(const char *name) {
    if (!name || *name == '\0') return 0;
    if (!isalpha((unsigned char)*name) && *name != '_') return 0;
    name++;
    while (*name) {
        if (!isalnum((unsigned char)*name) && *name != '_') return 0;
        name++;
    }
    return 1;
}

int builtin_export(int argc, char **argv) {
    if (argc < 2) {
        /* Print all environment variables if export with no args */
        extern char **environ;
        for (char **env = environ; *env != NULL; env++) {
            printf("declare -x %s\n", *env);
        }
        return 0;
    }

    int status = 0;
    for (int i = 1; i < argc; i++) {
        char *arg = strdup(argv[i]);
        char *eq = strchr(arg, '=');
        char *name = arg;
        char *val = NULL;

        if (eq) {
            *eq = '\0';
            val = eq + 1;
            /* Strip enclosing quotes if user passed VAR="value" */
            size_t vlen = strlen(val);
            if (vlen >= 2 && ((val[0] == '"' && val[vlen - 1] == '"') ||
                              (val[0] == '\'' && val[vlen - 1] == '\''))) {
                val[vlen - 1] = '\0';
                val++;
            }
        } else {
            /* export VAR without '=' */
            const char *existing = getenv(name);
            val = existing ? (char *)existing : "";
        }

        if (!is_valid_identifier(name)) {
            fprintf(stderr, "export: '%s': not a valid identifier\n", name);
            status = 1;
        } else {
            if (setenv(name, val, 1) != 0) {
                perror("export: setenv");
                status = 1;
            } else {
                printf("[Exported] %s=\"%s\"\n", name, val);
            }
        }
        free(arg);
    }
    return status;
}

int builtin_exit(int argc, char **argv, int *should_exit, int *exit_code) {
    *should_exit = 1;
    *exit_code = 0;

    if (argc > 1) {
        char *endptr;
        long code = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "exit: %s: numeric argument required\n", argv[1]);
            *exit_code = 2;
        } else {
            *exit_code = (int)(code & 0xFF);
        }
    }
    return 0;
}

void save_shell_state(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    if (!f) return;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        fprintf(f, "LAST_PWD=%s\n", cwd);
    }
    fprintf(f, "EXIT_TIME=%ld\n", (long)time(NULL));
    fclose(f);
}
