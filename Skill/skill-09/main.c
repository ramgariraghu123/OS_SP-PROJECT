#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cd_builtin.h"

/*
 * OSSP Skill 09: cd Command, Path Validation, Working Directory Changes,
 * and Previous Directory Handling.
 */

typedef int (*BuiltinHandler)(int argc, char **argv);

typedef struct {
    const char *name;
    BuiltinHandler handler;
    const char *help;
} BuiltinItem;

static int cmd_help(int argc, char **argv);

static BuiltinItem dispatch_table[] = {
    {"cd",   builtin_cd,  "Change working directory: cd [path | ~ | -]"},
    {"pwd",  builtin_pwd, "Print current working directory"},
    {"help", cmd_help,    "List available commands"},
    {NULL,   NULL,        NULL}
};

static int cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("\nAvailable Commands:\n");
    for (int i = 0; dispatch_table[i].name; i++) {
        printf("  %-6s - %s\n", dispatch_table[i].name, dispatch_table[i].help);
    }
    printf("\n");
    return 0;
}

int dispatch(int argc, char **argv) {
    if (argc == 0 || !argv || !argv[0]) return 0;
    for (int i = 0; dispatch_table[i].name; i++) {
        if (strcmp(argv[0], dispatch_table[i].name) == 0) {
            return dispatch_table[i].handler(argc, argv);
        }
    }
    fprintf(stderr, "shell: command not found: %s\n", argv[0]);
    return 127;
}

void execute_test_command(const char *cmdline) {
    printf("cd-shell> %s\n", cmdline);
    char buf[256];
    strncpy(buf, cmdline, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *argv[16];
    int argc = 0;
    char *tok = strtok(buf, " \t");
    while (tok && argc < 15) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    argv[argc] = NULL;

    if (argc > 0) {
        dispatch(argc, argv);
    }
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 09: cd Built-in & Directory State Tracker\n");
    printf("============================================================\n");

    init_pwd_environment();

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("[Running Automated Directory Navigation Tests]\n");
        execute_test_command("pwd");
        execute_test_command("cd /tmp");
        execute_test_command("pwd");
        execute_test_command("cd -"); /* Returns to previous */
        execute_test_command("pwd");
        execute_test_command("cd ..");
        execute_test_command("pwd");
        execute_test_command("cd -"); /* Returns again */
        execute_test_command("pwd");
        execute_test_command("cd /nonexistent_folder_abc"); /* Error test */
        return 0;
    }

    char buffer[1024];
    while (1) {
        char *pwd = getenv("PWD");
        printf("[%s]$ ", pwd ? pwd : "myshell");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        char *argv[32];
        int c = 0;
        char *tok = strtok(buffer, " \t");
        while (tok && c < 31) {
            argv[c++] = tok;
            tok = strtok(NULL, " \t");
        }
        argv[c] = NULL;
        if (c > 0) dispatch(c, argv);
    }

    printf("Exiting Skill 09.\n");
    return 0;
}
