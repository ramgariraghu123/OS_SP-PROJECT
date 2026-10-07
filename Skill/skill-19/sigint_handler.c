#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "sigint_handler.h"

volatile sig_atomic_t fg_pid = 0;

static void handle_sigint(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        /* Forward SIGINT to foreground process group */
        kill(-fg_pid, SIGINT);
    } else {
        /* Async-signal safe prompt redisplay */
        const char msg[] = "\n^C\nsigint-shell> ";
        if (write(STDOUT_FILENO, msg, sizeof(msg) - 1) < 0) { /* suppress unused */ }


    }
}

void setup_sigint_handler(void) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* Automatically restart interrupted system calls */
    sigaction(SIGINT, &sa, NULL);
}

void set_foreground_pid(pid_t pid) {
    fg_pid = pid;
}

pid_t get_foreground_pid(void) {
    return fg_pid;
}
