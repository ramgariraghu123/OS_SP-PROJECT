/**
 * Skill 13: Multi-Stage Pipelines & Descriptor Management
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Arbitrary N-stage pipeline execution (cmd1 | cmd2 | ... | cmdN)
 * - Dynamic allocation and management of (N-1) pipe descriptor pairs
 * - Wiring stdin and stdout across stages using dup2()
 * - Clean descriptor closing in both parent and children to prevent deadlocks
 * - Reaping all child processes using waitpid()
 * - Error propagation and resource reclamation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

typedef struct {
    char **argv;
} CommandStage;

// Executes an N-command pipeline
int execute_pipeline(CommandStage stages[], int num_stages) {
    if (num_stages <= 0) return 0;

    printf("\n[Multi-Pipe] Executing %d-stage pipeline:\n  ", num_stages);
    for (int i = 0; i < num_stages; i++) {
        for (int j = 0; stages[i].argv[j]; j++) {
            printf("%s ", stages[i].argv[j]);
        }
        if (i < num_stages - 1) printf("| ");
    }
    printf("\n");
    fflush(stdout);

    int num_pipes = num_stages - 1;
    int (*pipes)[2] = malloc(sizeof(int[2]) * (num_pipes > 0 ? num_pipes : 1));
    pid_t *pids = malloc(sizeof(pid_t) * num_stages);

    // Create all required pipes
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe creation failed");
            free(pipes);
            free(pids);
            return -1;
        }
    }

    // Launch all process stages
    for (int i = 0; i < num_stages; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork failed");
            free(pipes);
            free(pids);
            return -1;
        }

        if (pids[i] == 0) {
            // Child stage i

            // If not the first stage, redirect STDIN from previous pipe
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2 stdin failed");
                    exit(EXIT_FAILURE);
                }
            }

            // If not the last stage, redirect STDOUT to current pipe
            if (i < num_stages - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 stdout failed");
                    exit(EXIT_FAILURE);
                }
            }

            // Close ALL pipe ends in child
            for (int j = 0; j < num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(stages[i].argv[0], stages[i].argv);
            fprintf(stderr, "execvp '%s' failed: %s\n", stages[i].argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // Parent closes ALL pipe descriptors so reading children receive EOF
    for (int i = 0; i < num_pipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes in the pipeline
    int last_status = 0;
    for (int i = 0; i < num_stages; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == num_stages - 1) {
            last_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        }
    }

    free(pipes);
    free(pids);
    printf("[Multi-Pipe] Pipeline finished with exit code %d\n", last_status);
    return last_status;
}

int main() {
    printf("[Skill 13] Multi-Stage Pipeline Execution\n");

    // Test 1: 3-Stage Pipeline (ls -1 /usr/include | grep stdio | sort -r)
    char *s1_1[] = {"ls", "-1", "/usr/include", NULL};
    char *s1_2[] = {"grep", "stdio", NULL};
    char *s1_3[] = {"sort", "-r", NULL};
    CommandStage pipe1[] = { {s1_1}, {s1_2}, {s1_3} };
    execute_pipeline(pipe1, 3);

    // Test 2: 4-Stage Pipeline (cat /etc/passwd | cut -d: -f1 | sort | head -n 5)
    char *s2_1[] = {"cat", "/etc/passwd", NULL};
    char *s2_2[] = {"cut", "-d:", "-f1", NULL};
    char *s2_3[] = {"sort", NULL};
    char *s2_4[] = {"head", "-n", "5", NULL};
    CommandStage pipe2[] = { {s2_1}, {s2_2}, {s2_3}, {s2_4} };
    execute_pipeline(pipe2, 4);

    // Test 3: 5-Stage Pipeline (seq 1 100 | grep 5 | awk '{print $1*2}' | sort -nr | head -n 5)
    char *s3_1[] = {"seq", "1", "100", NULL};
    char *s3_2[] = {"grep", "5", NULL};
    char *s3_3[] = {"awk", "{print $1*2}", NULL};
    char *s3_4[] = {"sort", "-nr", NULL};
    char *s3_5[] = {"head", "-n", "5", NULL};
    CommandStage pipe3[] = { {s3_1}, {s3_2}, {s3_3}, {s3_4}, {s3_5} };
    execute_pipeline(pipe3, 5);

    printf("\n[Skill 13] All multi-stage pipeline tests executed successfully.\n");
    return EXIT_SUCCESS;
}
