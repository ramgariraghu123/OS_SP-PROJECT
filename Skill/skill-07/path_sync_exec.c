/**
 * Skill 07: Process Synchronization & PATH Resolution
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Parsing and traversing the PATH environment variable
 * - File permissions and executable verification using access(..., X_OK)
 * - Absolute vs relative vs PATH-resolved command location
 * - Spawning child process via fork()
 * - Executing resolved path directly using execv()
 * - Process synchronization and detailed exit status inspection using waitpid()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

#define MAX_PATH_LEN 1024

// Resolves a command name to an absolute executable path using the PATH variable
char *resolve_command_path(const char *command) {
    if (!command || strlen(command) == 0) return NULL;

    // If the command contains a slash '/', check it directly
    if (strchr(command, '/') != NULL) {
        if (access(command, X_OK) == 0) {
            return strdup(command);
        }
        return NULL;
    }

    const char *path_env = getenv("PATH");
    if (!path_env) {
        path_env = "/usr/bin:/bin:/usr/sbin:/sbin";
    }

    char *path_copy = strdup(path_env);
    char *dir = strtok(path_copy, ":");
    char candidate[MAX_PATH_LEN];

    while (dir != NULL) {
        snprintf(candidate, sizeof(candidate), "%s/%s", dir, command);

        // Check if file exists and has execute permission
        if (access(candidate, X_OK) == 0) {
            free(path_copy);
            return strdup(candidate);
        }

        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL; // Not found in any PATH directory
}

// Fork, execute resolved command with execv, and synchronize with waitpid
int run_resolved_command(char *const argv[]) {
    if (!argv || !argv[0]) return -1;

    char *resolved_path = resolve_command_path(argv[0]);
    if (!resolved_path) {
        fprintf(stderr, "[Resolution Error] Command '%s' not found or not executable in PATH\n", argv[0]);
        return 127;
    }

    printf("[PATH Resolver] Command '%s' resolved to -> '%s'\n", argv[0], resolved_path);
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        free(resolved_path);
        return -1;
    }

    if (pid == 0) {
        // Child executes resolved binary directly with execv
        printf("[Child PID %d] Executing '%s' via execv()...\n", getpid(), resolved_path);
        fflush(stdout);

        execv(resolved_path, argv);

        // Reached only on error
        perror("execv failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent synchronizes with child
        int status;
        printf("[Parent PID %d] Waiting for child PID %d...\n", getpid(), pid);
        fflush(stdout);

        pid_t waited_pid = waitpid(pid, &status, 0);
        free(resolved_path);

        if (waited_pid == -1) {
            perror("waitpid failed");
            return -1;
        }

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("[Parent] Child PID %d exited normally with code: %d\n", waited_pid, exit_code);
            return exit_code;
        } else if (WIFSIGNALED(status)) {
            int term_sig = WTERMSIG(status);
            printf("[Parent] Child PID %d killed by signal: %d\n", waited_pid, term_sig);
            return 128 + term_sig;
        } else if (WIFSTOPPED(status)) {
            printf("[Parent] Child PID %d stopped by signal: %d\n", waited_pid, WSTOPSIG(status));
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    printf("[Skill 07] Process Synchronization and PATH Resolution\n");

    if (argc > 1) {
        return run_resolved_command(&argv[1]);
    }

    // Automated Demonstration Cases
    printf("\n=== Test 1: Standard PATH command (whoami) ===\n");
    char *cmd1[] = {"whoami", NULL};
    run_resolved_command(cmd1);

    printf("\n=== Test 2: Standard PATH command with arguments (uname -r) ===\n");
    char *cmd2[] = {"uname", "-r", NULL};
    run_resolved_command(cmd2);

    printf("\n=== Test 3: Absolute path command (/bin/date) ===\n");
    char *cmd3[] = {"/bin/date", NULL};
    run_resolved_command(cmd3);

    printf("\n=== Test 4: Missing command resolution test ===\n");
    char *cmd4[] = {"random_nonexistent_command", NULL};
    run_resolved_command(cmd4);

    printf("\n[Skill 07] Process synchronization and PATH resolution tests complete.\n");
    return EXIT_SUCCESS;
}
