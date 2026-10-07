#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "builtins.h"

/*
 * OSSP Skill 10: pwd, exit, cleanup/state handling, and export/environment
 * variable functionality.
 */

void execute_child_check(const char *varname) {
    printf("  [Spawning child process to verify inheritance of $%s]:\n", varname);
    pid_t pid = fork();
    if (pid == 0) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "echo \"  -> Child process confirms: $%s='\"$%s\"'\"", varname, varname);
        char *args[] = {"/bin/sh", "-c", cmd, NULL};
        execv(args[0], args);
        exit(1);
    } else {
        int st;
        waitpid(pid, &st, 0);
    }
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 10: pwd, export, exit & State Management\n");
    printf("============================================================\n");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("\n--- Test 1: pwd command ---\n");
        char *pwd_args[] = {"pwd", NULL};
        builtin_pwd(1, pwd_args);

        printf("\n--- Test 2: Valid export with value ---\n");
        char *exp_args1[] = {"export", "LAB_COURSE=OSSP_2026", "STUDENT_ID=2520030199", NULL};
        builtin_export(3, exp_args1);
        execute_child_check("LAB_COURSE");

        printf("\n--- Test 3: Invalid identifier validation ---\n");
        char *exp_args2[] = {"export", "123INVALID=test", "VALID_VAR=passed", NULL};
        builtin_export(3, exp_args2);

        printf("\n--- Test 4: Exit command with custom code and state save ---\n");
        int should_exit = 0;
        int exit_code = 0;
        char *exit_args[] = {"exit", "42", NULL};
        builtin_exit(2, exit_args, &should_exit, &exit_code);
        if (should_exit) {
            save_shell_state(".shell_state");
            printf("[Shell State Saved to .shell_state] Exiting with code: %d\n", exit_code);
            return exit_code;
        }
        return 0;
    }

    char line[1024];
    while (1) {
        printf("env-shell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) == 0) continue;

        char *argv[32];
        int count = 0;
        char *tok = strtok(line, " \t");
        while (tok && count < 31) {
            argv[count++] = tok;
            tok = strtok(NULL, " \t");
        }
        argv[count] = NULL;
        if (count == 0) continue;

        if (strcmp(argv[0], "pwd") == 0) {
            builtin_pwd(count, argv);
        } else if (strcmp(argv[0], "export") == 0) {
            builtin_export(count, argv);
        } else if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0) {
            int should_exit = 0;
            int exit_code = 0;
            builtin_exit(count, argv, &should_exit, &exit_code);
            if (should_exit) {
                save_shell_state(".shell_state");
                printf("[Shell] Exiting with code %d. State saved.\n", exit_code);
                return exit_code;
            }
        } else {
            /* External fallback */
            pid_t p = fork();
            if (p == 0) {
                execvp(argv[0], argv);
                perror("execvp");
                exit(127);
            } else {
                int st;
                waitpid(p, &st, 0);
            }
        }
    }

    save_shell_state(".shell_state");
    return 0;
}
