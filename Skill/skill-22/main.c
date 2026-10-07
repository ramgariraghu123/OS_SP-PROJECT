#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"

/*
 * OSSP Skill 22: Valgrind Memory Checking, Leak Fixing, and Automated Testing.
 */

int main(void) {
    Shell sh;
    shell_init(&sh);

    char buffer[1024];
    while (sh.is_running) {
        if (isatty(STDIN_FILENO)) {
            printf("clean-shell> ");
            fflush(stdout);
        }

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        if (strlen(buffer) == 0) continue;

        shell_execute(&sh, buffer);
    }

    shell_cleanup(&sh);
    return sh.last_status;
}
