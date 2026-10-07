/**
 * Skill 06: Escape Sequences & Process Execution
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Handling backslash escape sequences (\ , \|, \<, \>, \&, \\, \")
 * - Distinguishing escaped metacharacters from shell operators
 * - Argument vector generation from escaped inputs
 * - Forking child processes via fork()
 * - Executing binary programs via execvp()
 * - Error propagation using perror() and exit status checking via waitpid()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_ARG_LEN 512

typedef struct {
    char *argv[MAX_ARGS];
    int argc;
} ParsedCommand;

void parsed_cmd_init(ParsedCommand *cmd) {
    cmd->argc = 0;
    for (int i = 0; i < MAX_ARGS; i++) {
        cmd->argv[i] = NULL;
    }
}

void parsed_cmd_free(ParsedCommand *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
        cmd->argv[i] = NULL;
    }
    cmd->argc = 0;
}

// Parse an input line honoring backslash escape sequences
int parse_escaped_line(const char *input, ParsedCommand *cmd) {
    parsed_cmd_init(cmd);
    size_t len = strlen(input);
    size_t i = 0;

    char current_arg[MAX_ARG_LEN];
    size_t arg_idx = 0;
    bool in_arg = false;

    while (i < len) {
        char c = input[i];

        if (c == '\\') {
            // Escape next character
            i++;
            if (i < len) {
                char escaped_char = input[i];
                if (arg_idx < MAX_ARG_LEN - 1) {
                    current_arg[arg_idx++] = escaped_char;
                    in_arg = true;
                }
                i++;
            }
        } else if (c == ' ' || c == '\t' || c == '\n') {
            if (in_arg) {
                current_arg[arg_idx] = '\0';
                if (cmd->argc < MAX_ARGS - 1) {
                    cmd->argv[cmd->argc++] = strdup(current_arg);
                }
                arg_idx = 0;
                in_arg = false;
            }
            i++;
        } else {
            if (arg_idx < MAX_ARG_LEN - 1) {
                current_arg[arg_idx++] = c;
                in_arg = true;
            }
            i++;
        }
    }

    if (in_arg) {
        current_arg[arg_idx] = '\0';
        if (cmd->argc < MAX_ARGS - 1) {
            cmd->argv[cmd->argc++] = strdup(current_arg);
        }
    }

    cmd->argv[cmd->argc] = NULL;
    return cmd->argc;
}

// Execute the parsed command in a child process
int execute_command(ParsedCommand *cmd) {
    if (cmd->argc == 0) return 0;

    printf("[Parent PID %d] Forking child to execute: '%s' with %d arguments\n",
           getpid(), cmd->argv[0], cmd->argc);
    for (int i = 0; i < cmd->argc; i++) {
        printf("  argv[%d] = \"%s\"\n", i, cmd->argv[i]);
    }
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork error");
        return -1;
    }

    if (pid == 0) {
        // Child process
        printf("[Child PID %d] Invoking execvp(\"%s\", ...)\n", getpid(), cmd->argv[0]);
        fflush(stdout);

        execvp(cmd->argv[0], cmd->argv);

        // Reached only if execvp fails
        fprintf(stderr, "[Child Error] execvp failed for '%s': %s\n",
                cmd->argv[0], strerror(errno));
        exit(errno == ENOENT ? 127 : 126);
    } else {
        // Parent process waits for child
        int status;
        pid_t waited = waitpid(pid, &status, 0);

        if (waited < 0) {
            perror("waitpid error");
            return -1;
        }

        if (WIFEXITED(status)) {
            printf("[Parent] Child %d terminated with exit code %d\n",
                   waited, WEXITSTATUS(status));
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            printf("[Parent] Child %d terminated by signal %d\n",
                   waited, WTERMSIG(status));
            return 128 + WTERMSIG(status);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    printf("[Skill 06] Escape Sequence Parsing and Command Execution\n");

    if (argc > 1) {
        // Run with command from command line argument
        char combined[1024] = {0};
        for (int i = 1; i < argc; i++) {
            strcat(combined, argv[i]);
            if (i < argc - 1) strcat(combined, " ");
        }
        ParsedCommand cmd;
        parse_escaped_line(combined, &cmd);
        int res = execute_command(&cmd);
        parsed_cmd_free(&cmd);
        return res;
    }

    // Automated test cases
    const char *test_commands[] = {
        "echo Hello\\ World\\ from\\ Escaped\\ String",
        "echo Characters\\ like\\ \\|\\ and\\ \\<\\ and\\ \\>\\ are\\ escaped\\ literally",
        "printf Argument\\ 1:%s\\n\\ Argument\\ 2:%s\\n Foo Bar",
        "nonexistent_command_xyz"
    };

    int num_tests = sizeof(test_commands) / sizeof(test_commands[0]);
    for (int i = 0; i < num_tests; i++) {
        printf("\n--------------------------------------------------\n");
        printf("Test Case %d: Input = [%s]\n", i + 1, test_commands[i]);
        ParsedCommand cmd;
        parse_escaped_line(test_commands[i], &cmd);
        execute_command(&cmd);
        parsed_cmd_free(&cmd);
    }

    printf("\n[Skill 06] All escape execution tests completed successfully.\n");
    return EXIT_SUCCESS;
}
