#ifndef BUILTINS_H
#define BUILTINS_H

int builtin_pwd(int argc, char **argv);
int builtin_export(int argc, char **argv);
int builtin_exit(int argc, char **argv, int *should_exit, int *exit_code);
void save_shell_state(const char *filepath);

#endif /* BUILTINS_H */
