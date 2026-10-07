/**
 * Skill 12: Single Pipe Execution & Inter-Process Communication (IPC)
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Unidirectional IPC pipe creation via pipe()
 * - File descriptor duplication and stream redirection using dup2()
 * - Proper closing of unused pipe file descriptors to prevent reader deadlocks
 * - Two-child concurrent process coordination (Writer & Reader)
 * - Program execution via execvp()
 * - Parent process synchronization using waitpid() for both children
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

// Execute two commands connected by a single pipe: cmd1 | cmd2
int execute_single_pipe(char *const cmd1[], char *const cmd2[]) {
    printf("\n[Pipeline Manager] Launching: ");
    for (int i = 0; cmd1[i]; i++) printf("%s ", cmd1[i]);
    printf("| ");
    for (int i = 0; cmd2[i]; i++) printf("%s ", cmd2[i]);
    printf("\n");
    fflush(stdout);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return -1;
    }

    // Fork Child 1 (Left command - Writer)
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork pid1 failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid1 == 0) {
        // Child 1: Connect STDOUT to pipe write end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2 child1 failed");
            exit(EXIT_FAILURE);
        }
        // Close both pipe ends in child
        close(pipefd[0]);
        close(pipefd[1]);

        execvp(cmd1[0], cmd1);
        perror("execvp cmd1 failed");
        exit(EXIT_FAILURE);
    }

    // Fork Child 2 (Right command - Reader)
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork pid2 failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid2 == 0) {
        // Child 2: Connect STDIN to pipe read end
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2 child2 failed");
            exit(EXIT_FAILURE);
        }
        // Close both pipe ends in child
        close(pipefd[0]);
        close(pipefd[1]);

        execvp(cmd2[0], cmd2);
        perror("execvp cmd2 failed");
        exit(EXIT_FAILURE);
    }

    // Parent MUST close both pipe ends so Reader receives EOF
    close(pipefd[0]);
    close(pipefd[1]);

    // Parent waits for both children
    int status1, status2;
    pid_t w1 = waitpid(pid1, &status1, 0);
    pid_t w2 = waitpid(pid2, &status2, 0);

    printf("[Parent] Reaped Writer (PID %d) status: %d | Reader (PID %d) status: %d\n",
           w1, WEXITSTATUS(status1), w2, WEXITSTATUS(status2));

    return WEXITSTATUS(status2);
}

int main(int argc, char *argv[]) {
    printf("[Skill 12] Single Pipe Execution & Inter-Process Communication\n");

    if (argc >= 3) {
        // Run user provided arguments: e.g. ./single_pipe_exec uname -a \| grep Linux
        // For simplicity, partition on "|" if present in argv
        int split_idx = -1;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "|") == 0) {
                split_idx = i;
                break;
            }
        }
        if (split_idx > 1 && split_idx < argc - 1) {
            argv[split_idx] = NULL;
            char **cmd1 = &argv[1];
            char **cmd2 = &argv[split_idx + 1];
            return execute_single_pipe(cmd1, cmd2);
        }
    }

    // Automated Demonstration Cases
    printf("\n=== Case 1: echo 'hello world systems programming' | tr 'a-z' 'A-Z' ===\n");
    char *cmd1_1[] = {"echo", "hello world systems programming", NULL};
    char *cmd1_2[] = {"tr", "a-z", "A-Z", NULL};
    execute_single_pipe(cmd1_1, cmd1_2);

    printf("\n=== Case 2: ls -1 /usr/include | grep stdio ===\n");
    char *cmd2_1[] = {"ls", "-1", "/usr/include", NULL};
    char *cmd2_2[] = {"grep", "stdio", NULL};
    execute_single_pipe(cmd2_1, cmd2_2);

    printf("\n=== Case 3: uname -s | wc -c ===\n");
    char *cmd3_1[] = {"uname", "-s", NULL};
    char *cmd3_2[] = {"wc", "-c", NULL};
    execute_single_pipe(cmd3_1, cmd3_2);

    printf("\n[Skill 12] Single pipe execution tests completed successfully.\n");
    return EXIT_SUCCESS;
}
