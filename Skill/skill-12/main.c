#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pipe_exec.h"

/*
 * OSSP Skill 12: Pipes, Connected Commands, File Descriptors,
 * Multiple Pipes, and Synchronization.
 */

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

void run_test_pipe(const char *cmdline) {
    printf("------------------------------------------------------------\n");
    printf("Executing Pipeline: %s\n", cmdline);

    char *copy = strdup(cmdline);
    char *saveptr;
    char *stage = strtok_r(copy, "|", &saveptr);

    CommandArgv cmds[16];
    int num_cmds = 0;

    while (stage && num_cmds < 16) {
        char *trimmed = trim(stage);
        cmds[num_cmds].argv = NULL;
        cmds[num_cmds].argc = 0;

        char *tok_save;
        char *tok = strtok_r(trimmed, " \t", &tok_save);
        while (tok) {
            cmds[num_cmds].argv = (char **)realloc(cmds[num_cmds].argv,
                                                  (cmds[num_cmds].argc + 2) * sizeof(char *));
            cmds[num_cmds].argv[cmds[num_cmds].argc++] = strdup(tok);
            cmds[num_cmds].argv[cmds[num_cmds].argc] = NULL;
            tok = strtok_r(NULL, " \t", &tok_save);
        }

        num_cmds++;
        stage = strtok_r(NULL, "|", &saveptr);
    }

    int rc = execute_pipeline(cmds, num_cmds);
    printf("Pipeline completed with final exit code: %d\n", rc);

    /* Cleanup */
    for (int i = 0; i < num_cmds; i++) {
        for (int j = 0; j < cmds[i].argc; j++) {
            free(cmds[i].argv[j]);
        }
        free(cmds[i].argv);
    }
    free(copy);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 12: Multi-Stage Pipeline Execution\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("\n[Test 1: Two-Stage Pipeline (ls -1 | wc -l)]\n");
        run_test_pipe("ls -1 | wc -l");

        printf("\n[Test 2: Three-Stage Pipeline (cat /etc/passwd | cut -d: -f1 | head -n 5)]\n");
        run_test_pipe("cat /etc/passwd | cut -d: -f1 | head -n 5");

        printf("\n[Test 3: Four-Stage Pipeline (seq 1 30 | grep 2 | sort -r | head -n 4)]\n");
        run_test_pipe("seq 1 30 | grep 2 | sort -r | head -n 4");
        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("pipe-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        run_test_pipe(buffer);
    }

    printf("Exiting Skill 12.\n");
    return 0;
}
