#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <signal.h>

#define MAX_HIST 30

typedef struct {
    char *history[MAX_HIST];
    int hist_count;
    int last_status;
    int is_running;
} Shell;

void shell_init(Shell *sh);
void shell_execute(Shell *sh, const char *line);
void shell_cleanup(Shell *sh);

#endif /* SHELL_H */
