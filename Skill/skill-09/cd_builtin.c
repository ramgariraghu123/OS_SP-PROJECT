#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "cd_builtin.h"

void init_pwd_environment(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        setenv("PWD", cwd, 1);
    }
}

int builtin_pwd(int argc, char **argv) {
    (void)argc;
    (void)argv;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd");
        return 1;
    }
}

int builtin_cd(int argc, char **argv) {
    char current_cwd[PATH_MAX];
    if (!getcwd(current_cwd, sizeof(current_cwd))) {
        perror("cd: getcwd");
        return 1;
    }

    const char *target = NULL;
    int print_new_dir = 0;

    if (argc < 2 || strcmp(argv[1], "~") == 0) {
        /* Navigate to HOME */
        target = getenv("HOME");
        if (!target) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    } else if (strcmp(argv[1], "-") == 0) {
        /* Navigate to OLDPWD */
        target = getenv("OLDPWD");
        if (!target) {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return 1;
        }
        print_new_dir = 1;
    } else {
        target = argv[1];
    }

    /* Change directory */
    if (chdir(target) != 0) {
        fprintf(stderr, "cd: %s: %s\n", target, strerror(errno));
        return 1;
    }

    /* Update OLDPWD and PWD */
    setenv("OLDPWD", current_cwd, 1);

    char new_cwd[PATH_MAX];
    if (getcwd(new_cwd, sizeof(new_cwd))) {
        setenv("PWD", new_cwd, 1);
        if (print_new_dir) {
            printf("%s\n", new_cwd);
        }
    }

    return 0;
}
