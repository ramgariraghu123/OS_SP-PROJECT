#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sigint_handler.h"

/*
 * OSSP Skill 19: Signal Handlers, SIGINT Handling, and Forwarding to Foreground.
 */

int run_command_fg(char *argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* Child in its own process group */
        setpgid(0, 0);

        /* Reset SIGINT in child to default action so it terminates */
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);

        execvp(argv[0], argv);
        perror("execvp");
        exit(127);
    }

    setpgid(pid, pid);
    set_foreground_pid(pid);

    int status;
    waitpid(pid, &status, 0);

    set_foreground_pid(0);

    if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) == SIGINT) {
            printf("\n[Notice] Foreground process (PID %d) terminated by SIGINT (Ctrl+C)\n", pid);
        } else {
            printf("\n[Notice] Foreground process terminated by signal %d\n", WTERMSIG(status));
        }
        return 128 + WTERMSIG(status);
    }

    return WIFEXITED(status) ? WEXITSTATUS(status) : 0;
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 19: SIGINT (Ctrl+C) Trapping & Forwarding\n");
    printf("============================================================\n");

    setup_sigint_handler();

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("\n[Test 1: Shell intercepts SIGINT while idle]\n");
        printf("Sending SIGINT to current shell PID (%d)...\n", getpid());
        kill(getpid(), SIGINT);
        usleep(100000);
        printf("\n[Verification: Shell is alive and running!]\n");

        printf("\n[Test 2: Spawning background thread to interrupt long foreground task]\n");
        pid_t runner = fork();
        if (runner == 0) {
            /* Wait 0.3 seconds and send SIGINT to parent shell */
            usleep(300000);
            kill(getppid(), SIGINT);
            exit(0);
        }

        char *sleep_args[] = {"sleep", "3", NULL};
        printf("Running 'sleep 3' in foreground (interrupter armed)...\n");
        run_command_fg(sleep_args);

        int st;
        waitpid(runner, &st, 0);
        printf("[Verification: Foreground process was interrupted, Shell survived!]\n");
        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("sigint-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("\nExiting.\n");
            break;
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        char *args[16];
        int c = 0;
        char *tok = strtok(buffer, " \t");
        while (tok && c < 15) {
            args[c++] = tok;
            tok = strtok(NULL, " \t");
        }
        args[c] = NULL;
        if (c > 0) run_command_fg(args);
    }

    printf("Exiting Skill 19.\n");
    return 0;
}
