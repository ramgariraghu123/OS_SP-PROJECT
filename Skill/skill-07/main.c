#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "path_resolver.h"

/*
 * OSSP Skill 07: PATH Parsing, Executable Resolution, and Child Monitoring
 * with waitpid().
 */

void execute_and_monitor(const char *cmd, char *args[]) {
    printf("------------------------------------------------------------\n");
    printf("Resolving command: \"%s\"\n", cmd);

    char err_msg[256] = {0};
    char *resolved = resolve_in_path(cmd, err_msg, sizeof(err_msg));

    if (!resolved) {
        printf("  [Resolution Failed] %s\n", err_msg);
        return;
    }

    printf("  [Resolved Location] -> %s\n", resolved);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(resolved);
        return;
    }

    if (pid == 0) {
        /* Child process: execute using exact resolved path */
        args[0] = resolved;
        execv(resolved, args);
        perror("execv failed");
        exit(1);
    } else {
        /* Parent process: monitor child state using waitpid */
        printf("  [Parent] Waiting for Child (PID: %d)...\n", pid);
        int status;
        pid_t waited = waitpid(pid, &status, WUNTRACED);

        if (waited == -1) {
            perror("waitpid");
        } else {
            printf("  [Monitoring Results for Child %d]:\n", waited);
            if (WIFEXITED(status)) {
                printf("    - Terminated Normally: YES\n");
                printf("    - Exit Code: %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("    - Terminated by Signal: YES\n");
                printf("    - Signal Number: %d (%s)\n",
                       WTERMSIG(status), strsignal(WTERMSIG(status)));
                #ifdef WCOREDUMP
                if (WCOREDUMP(status)) {
                    printf("    - Core Dumped: YES\n");
                }
                #endif
            } else if (WIFSTOPPED(status)) {
                printf("    - Stopped by Signal: %d (%s)\n",
                       WSTOPSIG(status), strsignal(WSTOPSIG(status)));
            }
        }
        free(resolved);
    }
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 07: PATH Resolution & waitpid() Monitoring\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        /* Test 1: Standard utility in /bin or /usr/bin */
        char *args1[] = {NULL, "-sh", ".", NULL};
        execute_and_monitor("du", args1);

        /* Test 2: Another standard utility */
        char *args2[] = {NULL, NULL};
        execute_and_monitor("whoami", args2);

        /* Test 3: Command with explicit path */
        char *args3[] = {NULL, "Explicit path execution works!", NULL};
        execute_and_monitor("/bin/echo", args3);

        /* Test 4: Non-existent command */
        char *args4[] = {NULL, NULL};
        execute_and_monitor("fake_binary_never_exists", args4);

        return 0;
    }

    char line[512];
    while (1) {
        printf("path-shell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;
        if (strlen(line) == 0) continue;

        char *cmd = strtok(line, " \t");
        if (!cmd) continue;

        char *args[16];
        args[0] = cmd;
        int arg_idx = 1;
        char *token;
        while ((token = strtok(NULL, " \t")) != NULL && arg_idx < 15) {
            args[arg_idx++] = token;
        }
        args[arg_idx] = NULL;

        execute_and_monitor(cmd, args);
    }

    printf("Exiting Skill 07.\n");
    return 0;
}
