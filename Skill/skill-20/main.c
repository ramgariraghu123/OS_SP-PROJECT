#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include "term_control.h"

/*
 * OSSP Skill 20: SIGTSTP, Stopped Jobs, Process Groups,
 * and Terminal Control Management.
 */

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

void launch_and_manage(TerminalJobManager *mgr, char *argv[], const char *raw_cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        /* Child process */
        setpgid(0, 0);

        /* Restore default signal actions for child */
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        execvp(argv[0], argv);
        perror("execvp");
        exit(127);
    }

    /* Parent process */
    setpgid(pid, pid);

    if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, pid);

    int status;
    waitpid(pid, &status, WUNTRACED);

    if (isatty(STDIN_FILENO)) tcsetpgrp(STDIN_FILENO, mgr->shell_pgid);

    if (WIFSTOPPED(status)) {
        int id = term_mgr_add(mgr, pid, raw_cmd, JOB_STOPPED);
        printf("\n[%d]+ Stopped                 %s (PGID: %d)\n", id, raw_cmd, pid);
    }
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 20: Process Groups & Terminal Control (SIGTSTP)\n");
    printf("============================================================\n");

    TerminalJobManager *mgr = term_mgr_init();

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("\n[Test 1: Launch long process and send SIGTSTP to test suspension]\n");
        pid_t p = fork();
        if (p == 0) {
            setpgid(0, 0);
            signal(SIGTSTP, SIG_DFL);
            while (1) { sleep(1); }
            exit(0);
        }
        setpgid(p, p);
        usleep(100000); /* Small delay to ensure child setpgid and signal handler configured */


        /* Simulate Ctrl+Z by sending SIGTSTP */
        kill(-p, SIGTSTP);
        int st;
        waitpid(p, &st, WUNTRACED);

        if (WIFSTOPPED(st)) {
            int jid = term_mgr_add(mgr, p, "sleep 100", JOB_STOPPED);
            printf("  Process stopped and saved to Job Table as Job [%d]\n", jid);
        }

        printf("\n[Test 2: Display active jobs via 'jobs']\n");
        term_mgr_list(mgr);

        printf("\n[Test 3: Resume in background via 'bg 1']\n");
        term_mgr_bg(mgr, 1);
        term_mgr_list(mgr);

        /* Clean child */
        kill(-p, SIGKILL);
        waitpid(p, &st, 0);
        term_mgr_destroy(mgr);
        printf("\nTerminal control and suspension workflow verified.\n");
        return 0;
    }

    char buffer[1024];
    while (1) {
        term_mgr_reap(mgr);

        printf("term-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        char *trimmed = trim(buffer);

        if (strcmp(trimmed, "jobs") == 0) {
            term_mgr_list(mgr);
            continue;
        }

        if (strncmp(trimmed, "fg", 2) == 0 && (trimmed[2] == ' ' || trimmed[2] == '\0')) {
            int id = 1;
            char *arg = trimmed + 2;
            while (*arg == ' ' || *arg == '%') arg++;
            if (*arg != '\0') id = atoi(arg);
            term_mgr_fg(mgr, id);
            continue;
        }

        if (strncmp(trimmed, "bg", 2) == 0 && (trimmed[2] == ' ' || trimmed[2] == '\0')) {
            int id = 1;
            char *arg = trimmed + 2;
            while (*arg == ' ' || *arg == '%') arg++;
            if (*arg != '\0') id = atoi(arg);
            term_mgr_bg(mgr, id);
            continue;
        }

        char *cmd_argv[16];
        int c = 0;
        char *tok = strtok(trimmed, " \t");
        while (tok && c < 15) {
            cmd_argv[c++] = tok;
            tok = strtok(NULL, " \t");
        }
        cmd_argv[c] = NULL;
        if (c == 0) continue;

        launch_and_manage(mgr, cmd_argv, buffer);
    }

    term_mgr_destroy(mgr);
    printf("Exiting Skill 20.\n");
    return 0;
}
