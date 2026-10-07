/**
 * Skill 01: Process Abstraction and Hierarchy
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Process creation via fork()
 * - Process abstraction and PID / PPID inspection
 * - Binary execution using execvp()
 * - Parent-child synchronization and exit status tracking using waitpid()
 * - Process tree and hierarchy visualization
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

void print_separator(const char *title) {
    printf("\n==================================================\n");
    printf("  %s\n", title);
    printf("==================================================\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    printf("[Skill 01] Process Abstraction and Hierarchy Demo\n");
    printf("Root Process PID: %d, Parent PID: %d\n", getpid(), getppid());
    fflush(stdout);

    print_separator("PART 1: Basic Process Hierarchy (fork & waitpid)");
    fflush(stdout);
    pid_t child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        // Inside Child Process
        printf("[Child 1] Running in Child Process!\n");
        printf("[Child 1] Child PID  : %d\n", getpid());
        printf("[Child 1] Parent PID : %d\n", getppid());
        printf("[Child 1] Exiting with status 42...\n");
        fflush(stdout);
        exit(42);
    } else {
        // Inside Parent Process
        printf("[Parent] Forked Child 1 with PID: %d\n", child_pid);
        fflush(stdout);
        int status;
        pid_t waited_pid = waitpid(child_pid, &status, 0);

        if (waited_pid == -1) {
            perror("waitpid failed");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status)) {
            printf("[Parent] Child 1 (%d) terminated normally with exit status: %d\n",
                   waited_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[Parent] Child 1 (%d) terminated by signal: %d\n",
                   waited_pid, WTERMSIG(status));
        }
        fflush(stdout);
    }

    print_separator("PART 2: Program Execution with execvp()");
    fflush(stdout);
    pid_t exec_child_pid = fork();

    if (exec_child_pid < 0) {
        perror("fork failed");
        return EXIT_FAILURE;
    }

    if (exec_child_pid == 0) {
        // Child executes a command
        printf("[Child 2] Executing command using execvp()...\n");
        char *cmd_args[4];
        if (argc > 1) {
            for (int i = 1; i < argc && i < 3; i++) {
                cmd_args[i - 1] = argv[i];
            }
            cmd_args[argc - 1] = NULL;
        } else {
            cmd_args[0] = "uname";
            cmd_args[1] = "-a";
            cmd_args[2] = NULL;
        }

        printf("[Child 2] Replacing process image with '%s'...\n", cmd_args[0]);
        fflush(stdout);
        execvp(cmd_args[0], cmd_args);

        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(exec_child_pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("[Parent] Child 2 completed with status %d\n", WEXITSTATUS(status));
        }
        fflush(stdout);
    }

    print_separator("PART 3: Multi-Process Tree Hierarchy");
    printf("[Parent] Spawning 3 child processes to form a process tree...\n");
    fflush(stdout);
    pid_t children[3];
    for (int i = 0; i < 3; i++) {
        children[i] = fork();
        if (children[i] < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        if (children[i] == 0) {
            printf("  -> [Child %d] PID: %d, PPID: %d - executing task...\n",
                   i + 1, getpid(), getppid());
            fflush(stdout);
            usleep(50000 * (i + 1));
            exit(10 + i);
        }
    }

    // Parent waits for all 3 children
    for (int i = 0; i < 3; i++) {
        int status;
        pid_t w = waitpid(children[i], &status, 0);
        if (WIFEXITED(status)) {
            printf("[Parent] Reaped Child PID %d with exit code %d\n", w, WEXITSTATUS(status));
        }
    }

    printf("\n[Skill 01] Process hierarchy demonstration completed successfully.\n");
    return EXIT_SUCCESS;
}
