#ifndef SIGINT_HANDLER_H
#define SIGINT_HANDLER_H

#include <signal.h>
#include <sys/types.h>

extern volatile sig_atomic_t fg_pid;

void setup_sigint_handler(void);
void set_foreground_pid(pid_t pid);
pid_t get_foreground_pid(void);

#endif /* SIGINT_HANDLER_H */
