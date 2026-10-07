#ifndef SHELL_H
#define SHELL_H

#include "monitor.h"

typedef struct {
    int last_status;
    int is_running;
} StressShell;

void stress_shell_init(StressShell *sh);
void stress_shell_exec(StressShell *sh, const char *line);

#endif /* SHELL_H */
