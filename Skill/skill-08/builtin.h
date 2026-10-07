#ifndef BUILTIN_H
#define BUILTIN_H

typedef int (*BuiltinFunc)(int argc, char **argv, int *last_status);

typedef struct {
    const char *name;
    BuiltinFunc func;
    const char *description;
} BuiltinCommand;

/* Checks if a command is a built-in; if yes, executes it and returns 1, else returns 0 */
int dispatch_builtin(int argc, char **argv, int *last_status);
void print_builtin_help(void);

#endif /* BUILTIN_H */
