/**
 * Skill 10: Built-in Commands & Child Process Environment Inheritance
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Parsing and validating export syntax (export NAME=VALUE)
 * - Updating parent environment via setenv()
 * - Retrieving current working directory via getcwd() (pwd)
 * - Processing exit requests with custom exit codes
 * - Demonstrating environment inheritance across fork() and execvp()
 * - Proper error handling and resource cleanup
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

extern char **environ;

// Validate that environment variable name is valid identifier
bool is_valid_var_name(const char *name) {
    if (!name || !*name) return false;
    if (!isalpha((unsigned char)*name) && *name != '_') return false;
    for (size_t i = 1; name[i] != '\0'; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') return false;
    }
    return true;
}

// Built-in export
int builtin_export(int argc, char **argv) {
    if (argc < 2) {
        // List all exported environment variables
        printf("--- Exported Environment Variables ---\n");
        for (char **env = environ; *env != NULL; env++) {
            printf("declare -x %s\n", *env);
        }
        printf("---------------------------------------\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        char *arg = strdup(argv[i]);
        char *eq = strchr(arg, '=');

        if (eq) {
            *eq = '\0';
            char *var_name = arg;
            char *var_val = eq + 1;

            if (!is_valid_var_name(var_name)) {
                fprintf(stderr, "export: `%s': not a valid identifier\n", argv[i]);
                free(arg);
                return 1;
            }

            if (setenv(var_name, var_val, 1) != 0) {
                perror("setenv failed");
                free(arg);
                return 1;
            }
            printf("[export] %s=\"%s\"\n", var_name, var_val);
        } else {
            // Just export variable name if already in environment
            if (!is_valid_var_name(arg)) {
                fprintf(stderr, "export: `%s': not a valid identifier\n", arg);
                free(arg);
                return 1;
            }
            printf("[export] %s marked for export\n", arg);
        }
        free(arg);
    }
    return 0;
}

// Built-in pwd
int builtin_pwd(int argc, char **argv) {
    (void)argc; (void)argv;
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd getcwd failed");
        return 1;
    }
}

// Built-in exit
int builtin_exit(int argc, char **argv) {
    int code = 0;
    if (argc > 1) {
        code = atoi(argv[1]);
    }
    printf("[exit] Shell terminating with status code %d\n", code);
    return code;
}

// Verify that exported variable is inherited by child processes
int test_child_inheritance(const char *var_name) {
    printf("[Parent PID %d] Forking child to verify child receives $%s...\n",
           getpid(), var_name);
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {
        // In Child Process: check getenv() and run child binary
        char *val = getenv(var_name);
        printf("[Child PID %d] Environment lookup: $%s = \"%s\"\n",
               getpid(), var_name, val ? val : "<NOT SET>");
        fflush(stdout);

        // Execute /bin/sh to verify inheritance across execve
        char cmd_str[256];
        snprintf(cmd_str, sizeof(cmd_str), "echo \"[Subshell exec] Received $%s = \"$%s", var_name, var_name);
        char *args[] = {"/bin/sh", "-c", cmd_str, NULL};
        execvp(args[0], args);

        perror("execvp child error");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        printf("[Parent] Child completed with status %d\n", WEXITSTATUS(status));
    }
    return 0;
}

int main() {
    printf("[Skill 10] Built-in Commands & Child Process Environment Inheritance\n");

    printf("\n=== Test 1: Built-in pwd ===\n");
    builtin_pwd(0, NULL);

    printf("\n=== Test 2: Built-in export ===\n");
    char *exp1[] = {"export", "OSSP_LAB_TOKEN=X9988_TOKEN", "STUDENT_ID=2520030225"};
    builtin_export(3, exp1);

    printf("\n=== Test 3: Export invalid variable name ===\n");
    char *exp_bad[] = {"export", "123INVALID_NAME=data"};
    builtin_export(2, exp_bad);

    printf("\n=== Test 4: Verify Child Process Inheritance ===\n");
    test_child_inheritance("OSSP_LAB_TOKEN");
    test_child_inheritance("STUDENT_ID");

    printf("\n=== Test 5: Built-in exit Simulation ===\n");
    char *exit_cmd[] = {"exit", "0"};
    int exit_status = builtin_exit(2, exit_cmd);
    printf("Simulated exit returned code: %d\n", exit_status);

    printf("\n[Skill 10] Environment and built-in management tests completed successfully.\n");
    return EXIT_SUCCESS;
}
