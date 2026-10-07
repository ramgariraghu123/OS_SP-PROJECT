/**
 * Skill 08: Variable Expansion & Built-in Dispatch Table
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Environment variable detection and expansion ($VAR, $?, $$)
 * - Handling undefined variables gracefully (POSIX empty string)
 * - Dispatch table architecture with function pointers
 * - In-process execution of built-ins to maintain shell state
 * - Clean token updates and dynamic memory management
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

// Shell State
static int last_exit_status = 0;
static bool shell_running = true;

/* --- Variable Expansion Subsystem --- */
char *expand_variables(const char *input) {
    if (!input) return NULL;

    size_t cap = 256;
    size_t len = 0;
    char *out = (char *)malloc(cap);
    out[0] = '\0';

    size_t i = 0;
    size_t in_len = strlen(input);

    while (i < in_len) {
        if (input[i] == '$') {
            i++; // skip '$'
            if (i >= in_len) {
                // Trailing '$'
                if (len + 2 > cap) { cap *= 2; out = (char *)realloc(out, cap); }
                out[len++] = '$';
                out[len] = '\0';
                break;
            }

            if (input[i] == '?') {
                // Exit status
                char status_str[16];
                snprintf(status_str, sizeof(status_str), "%d", last_exit_status);
                size_t s_len = strlen(status_str);
                while (len + s_len + 1 > cap) { cap *= 2; out = (char *)realloc(out, cap); }
                strcat(out, status_str);
                len += s_len;
                i++;
            } else if (input[i] == '$') {
                // PID
                char pid_str[16];
                snprintf(pid_str, sizeof(pid_str), "%d", getpid());
                size_t p_len = strlen(pid_str);
                while (len + p_len + 1 > cap) { cap *= 2; out = (char *)realloc(out, cap); }
                strcat(out, pid_str);
                len += p_len;
                i++;
            } else {
                // Named variable
                size_t start = i;
                while (i < in_len && (isalnum((unsigned char)input[i]) || input[i] == '_')) {
                    i++;
                }
                size_t var_len = i - start;
                char var_name[128];
                if (var_len >= sizeof(var_name)) var_len = sizeof(var_name) - 1;
                strncpy(var_name, input + start, var_len);
                var_name[var_len] = '\0';

                const char *val = getenv(var_name);
                if (val) {
                    size_t v_len = strlen(val);
                    while (len + v_len + 1 > cap) { cap *= 2; out = (char *)realloc(out, cap); }
                    strcat(out, val);
                    len += v_len;
                }
                // Undefined variable expands to empty string
            }
        } else {
            if (len + 2 > cap) { cap *= 2; out = (char *)realloc(out, cap); }
            out[len++] = input[i++];
            out[len] = '\0';
        }
    }

    return out;
}

/* --- Built-in Dispatch Table Subsystem --- */
typedef int (*builtin_fn)(int argc, char **argv);

int builtin_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("%s%s", argv[i], (i == argc - 1) ? "" : " ");
    }
    printf("\n");
    return 0;
}

int builtin_set(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: set KEY=VALUE\n");
        return 1;
    }
    char *eq = strchr(argv[1], '=');
    if (!eq) {
        printf("Error: format must be KEY=VALUE\n");
        return 1;
    }
    *eq = '\0';
    char *key = argv[1];
    char *val = eq + 1;
    setenv(key, val, 1);
    printf("Set variable: %s=\"%s\"\n", key, val);
    return 0;
}

int builtin_status(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("[Shell State] PID: %d, Last Exit Status ($?): %d\n", getpid(), last_exit_status);
    return 0;
}

int builtin_exit(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("Exiting shell...\n");
    shell_running = false;
    return 0;
}

int builtin_help(int argc, char **argv);

typedef struct {
    const char *name;
    builtin_fn func;
    const char *description;
} BuiltinCommand;

static BuiltinCommand dispatch_table[] = {
    {"echo",   builtin_echo,   "Print arguments to standard output with variable expansion"},
    {"set",    builtin_set,    "Define or update an environment variable (KEY=VALUE)"},
    {"status", builtin_status, "Display current shell state and PID"},
    {"help",   builtin_help,   "Display list of supported built-in commands"},
    {"exit",   builtin_exit,   "Terminate shell execution"},
    {NULL,     NULL,           NULL}
};

int builtin_help(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("Available Shell Built-in Commands (Dispatch Table):\n");
    for (int i = 0; dispatch_table[i].name != NULL; i++) {
        printf("  %-10s - %s\n", dispatch_table[i].name, dispatch_table[i].description);
    }
    return 0;
}

builtin_fn lookup_builtin(const char *name) {
    for (int i = 0; dispatch_table[i].name != NULL; i++) {
        if (strcmp(dispatch_table[i].name, name) == 0) {
            return dispatch_table[i].func;
        }
    }
    return NULL;
}

// Execute a command line
void execute_line(const char *line) {
    printf("\n>> Input Command: %s\n", line);
    char *expanded = expand_variables(line);
    printf("   Expanded Line: %s\n", expanded);

    // Split into tokens
    char *tokens[32];
    int token_count = 0;
    char *saveptr;
    char *tok = strtok_r(expanded, " \t\r\n", &saveptr);
    while (tok && token_count < 31) {
        tokens[token_count++] = tok;
        tok = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    tokens[token_count] = NULL;

    if (token_count == 0) {
        free(expanded);
        return;
    }

    builtin_fn handler = lookup_builtin(tokens[0]);
    if (handler) {
        printf("   [Dispatch Table] Executing built-in handler '%s' in-process\n", tokens[0]);
        last_exit_status = handler(token_count, tokens);
    } else {
        printf("   [Error] Unknown command '%s' (not in dispatch table)\n", tokens[0]);
        last_exit_status = 127;
    }

    free(expanded);
}

int main() {
    printf("[Skill 08] Variable Expansion & Built-in Dispatch Table\n");

    setenv("PROJECT_NAME", "OSSP-Shell", 1);
    setenv("DEVELOPER", "Raghuveer", 1);

    // Test cases
    execute_line("help");
    execute_line("echo Project: $PROJECT_NAME by Developer: $DEVELOPER");
    execute_line("echo Shell PID is $$ and previous status was $?");
    execute_line("echo Undefined variable expands to empty: [$NONEXISTENT_VAR]");
    execute_line("set GREETING=Hello_World");
    execute_line("echo Variable just set: $GREETING");
    execute_line("status");
    execute_line("invalid_cmd_test");
    execute_line("echo Status after error: $?");
    execute_line("exit");

    printf("\n[Skill 08] Dispatch table and expansion tests finished successfully.\n");
    return EXIT_SUCCESS;
}
