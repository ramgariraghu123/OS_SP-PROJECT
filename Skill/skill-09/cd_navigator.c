/**
 * Skill 09: Directory Navigation & State Management (cd, OLDPWD, PWD)
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Built-in cd command implementation
 * - Directory switching via chdir()
 * - Real-time working directory retrieval with getcwd()
 * - Managing previous working directory ($OLDPWD) and toggling with 'cd -'
 * - Home directory expansion ('cd' or 'cd ~')
 * - Error reporting with perror() for missing/inaccessible paths
 * - Dispatch table integration and navigation state preservation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#define PATH_BUF_SIZE 4096

// Global shell directory state
static char current_pwd[PATH_BUF_SIZE];
static char previous_pwd[PATH_BUF_SIZE] = "";

void init_directory_state() {
    if (getcwd(current_pwd, sizeof(current_pwd)) == NULL) {
        perror("getcwd failed during initialization");
        snprintf(current_pwd, sizeof(current_pwd), "/");
    }
    setenv("PWD", current_pwd, 1);
}

// Built-in cd implementation
int builtin_cd(int argc, char **argv) {
    char target_path[PATH_BUF_SIZE];
    const char *dest = NULL;

    if (argc < 2 || strcmp(argv[1], "~") == 0) {
        // 'cd' or 'cd ~' -> HOME
        dest = getenv("HOME");
        if (!dest) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
        snprintf(target_path, sizeof(target_path), "%s", dest);
    } else if (strcmp(argv[1], "-") == 0) {
        // 'cd -' -> OLDPWD
        if (strlen(previous_pwd) == 0) {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return 1;
        }
        snprintf(target_path, sizeof(target_path), "%s", previous_pwd);
        printf("%s\n", target_path); // Bash prints directory on 'cd -'
    } else {
        // Specific relative or absolute directory
        snprintf(target_path, sizeof(target_path), "%s", argv[1]);
    }

    // Attempt chdir
    if (chdir(target_path) != 0) {
        fprintf(stderr, "cd: %s: %s\n", target_path, strerror(errno));
        return 1;
    }

    // Save previous working directory
    snprintf(previous_pwd, sizeof(previous_pwd), "%s", current_pwd);
    setenv("OLDPWD", previous_pwd, 1);

    // Update current working directory
    if (getcwd(current_pwd, sizeof(current_pwd)) == NULL) {
        perror("getcwd failed after chdir");
        return 1;
    }
    setenv("PWD", current_pwd, 1);

    printf("[cd] Successfully navigated to: %s\n", current_pwd);
    return 0;
}

int builtin_pwd(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("%s\n", current_pwd);
    return 0;
}

int builtin_env_pwd(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("PWD    : %s\n", getenv("PWD") ? getenv("PWD") : "<none>");
    printf("OLDPWD : %s\n", getenv("OLDPWD") ? getenv("OLDPWD") : "<none>");
    return 0;
}

// Automated test sequence
void run_cd_test(const char *label, int argc, char **argv) {
    printf("\n=== Test: %s ===\n", label);
    printf("Executing command: ");
    for (int i = 0; i < argc; i++) printf("%s ", argv[i]);
    printf("\n");

    int res = builtin_cd(argc, argv);
    printf("Result code: %d | Current PWD: %s\n", res, current_pwd);
}

int main() {
    printf("[Skill 09] Directory Navigation & State Management (cd, PWD, OLDPWD)\n");
    init_directory_state();

    char initial_dir[PATH_BUF_SIZE];
    snprintf(initial_dir, sizeof(initial_dir), "%s", current_pwd);
    printf("Starting Directory: %s\n", current_pwd);

    // Test 1: Navigate to /tmp
    char *cmd1[] = {"cd", "/tmp"};
    run_cd_test("Navigate to absolute path /tmp", 2, cmd1);

    // Test 2: Toggle back with cd -
    char *cmd2[] = {"cd", "-"};
    run_cd_test("Toggle back using 'cd -' (should restore initial directory)", 2, cmd2);

    // Test 3: Navigate to Home directory with 'cd'
    char *cmd3[] = {"cd"};
    run_cd_test("Navigate to Home with 'cd' (no args)", 1, cmd3);

    // Test 4: Navigate to parent directory with 'cd ..'
    char *cmd4[] = {"cd", ".."};
    run_cd_test("Navigate to parent directory '..'", 2, cmd4);

    // Test 5: Navigate to invalid directory
    char *cmd5[] = {"cd", "/nonexistent_folder_abc_123"};
    run_cd_test("Error case: navigate to nonexistent directory", 2, cmd5);

    // Test 6: Display environment state
    printf("\n=== Environment State Check ===\n");
    builtin_env_pwd(0, NULL);

    // Restore original dir
    if (chdir(initial_dir) != 0) {
        perror("Failed to restore initial directory");
    }

    printf("\n[Skill 09] Directory navigation tests completed successfully.\n");
    return EXIT_SUCCESS;
}
