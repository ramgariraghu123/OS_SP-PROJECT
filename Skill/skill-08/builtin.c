#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "builtin.h"

static int builtin_echo(int argc, char **argv, int *last_status) {
    int newline = 1;
    int start = 1;

    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        newline = 0;
        start = 2;
    }

    for (int i = start; i < argc; i++) {
        printf("%s%s", argv[i], (i + 1 < argc) ? " " : "");
    }
    if (newline) printf("\n");
    fflush(stdout);

    *last_status = 0;
    return 0;
}

static int builtin_setvar(int argc, char **argv, int *last_status) {
    if (argc < 3) {
        fprintf(stderr, "Usage: setvar <NAME> <VALUE>\n");
        *last_status = 1;
        return 1;
    }
    if (setenv(argv[1], argv[2], 1) == 0) {
        printf("[Shell Built-in] Variable '%s' set to '%s'\n", argv[1], argv[2]);
        *last_status = 0;
        return 0;
    } else {
        perror("setenv");
        *last_status = 1;
        return 1;
    }
}

static int builtin_getvar(int argc, char **argv, int *last_status) {
    if (argc < 2) {
        fprintf(stderr, "Usage: getvar <NAME>\n");
        *last_status = 1;
        return 1;
    }
    const char *val = getenv(argv[1]);
    if (val) {
        printf("%s=%s\n", argv[1], val);
        *last_status = 0;
    } else {
        printf("[Shell Built-in] Variable '%s' is not set.\n", argv[1]);
        *last_status = 1;
    }
    return 0;
}

static int builtin_status(int argc, char **argv, int *last_status) {
    (void)argc;
    (void)argv;
    printf("[Shell State]\n");
    printf("  PID ($$):         %d\n", getpid());
    printf("  Last Status ($?): %d\n", *last_status);
    return 0;
}

static int builtin_help_cmd(int argc, char **argv, int *last_status);

/* The Dispatch Table */
static BuiltinCommand dispatch_table[] = {
    {"echo",   builtin_echo,     "Prints arguments to standard output (-n flag supported)"},
    {"setvar", builtin_setvar,   "Sets an environment variable: setvar <NAME> <VALUE>"},
    {"getvar", builtin_getvar,   "Gets the value of an environment variable: getvar <NAME>"},
    {"status", builtin_status,   "Displays internal shell status ($$ and $?)"},
    {"help",   builtin_help_cmd, "Displays list of supported built-in commands"},
    {NULL,     NULL,             NULL}
};

static int builtin_help_cmd(int argc, char **argv, int *last_status) {
    (void)argc;
    (void)argv;
    printf("\n=== Shell Built-in Dispatch Table ===\n");
    for (int i = 0; dispatch_table[i].name != NULL; i++) {
        printf("  %-10s : %s\n", dispatch_table[i].name, dispatch_table[i].description);
    }
    printf("=====================================\n\n");
    *last_status = 0;
    return 0;
}

void print_builtin_help(void) {
    int dummy = 0;
    builtin_help_cmd(0, NULL, &dummy);
}

int dispatch_builtin(int argc, char **argv, int *last_status) {
    if (argc == 0 || !argv || !argv[0]) return 0;

    for (int i = 0; dispatch_table[i].name != NULL; i++) {
        if (strcmp(argv[0], dispatch_table[i].name) == 0) {
            dispatch_table[i].func(argc, argv, last_status);
            return 1; /* Successfully matched and executed */
        }
    }
    return 0; /* Not a built-in */
}
