#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

/*
 * OSSP Skill 01: Process Abstraction, Fork, Exec, Parent-Child Relationships,
 * Process Inspection, and System Call Tracing.
 *
 * System calls demonstrated:
 *   - fork(): creates a child process by duplicating the calling process
 *   - getpid(): returns the process ID of the calling process
 *   - getppid(): returns the process ID of the parent process
 *   - execvp(): replaces current process image with a new process image
 *   - waitpid(): suspends execution of calling process until child status changes
 */

void inspect_process_proc(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("  [Info] Unable to read %s\n", path);
        return;
    }
    printf("\n--- /proc/%d/status (Key Fields) ---\n", pid);
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < 8) {
        if (strncmp(line, "Name:", 5) == 0 ||
            strncmp(line, "State:", 6) == 0 ||
            strncmp(line, "Pid:", 4) == 0 ||
            strncmp(line, "PPid:", 5) == 0 ||
            strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "Threads:", 8) == 0) {
            printf("  %s", line);
            count++;
        }
    }
    fclose(f);
    printf("------------------------------------\n");
}

void demo_fork_and_relationships(void) {
    printf("\n=== 1. Fork & Parent-Child Relationship Demo ===\n");
    pid_t my_pid = getpid();
    pid_t my_ppid = getppid();
    printf("[Parent Process] My PID: %d, My Parent PID (Bash/WSL): %d\n", my_pid, my_ppid);

    printf("[Parent Process] Calling fork()...\n");
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    } else if (pid == 0) {
        /* Child Process */
        printf("  [Child Process] Hello from Child!\n");
        printf("  [Child Process] Child PID: %d, Child PPID: %d\n", getpid(), getppid());
        printf("  [Child Process] Child is exiting with status code 7.\n");
        exit(7);
    } else {
        /* Parent Process */
        printf("[Parent Process] Forked child with PID: %d\n", pid);
        printf("[Parent Process] Waiting for child %d to finish using waitpid()...\n", pid);

        int status;
        pid_t waited_pid = waitpid(pid, &status, 0);

        if (waited_pid == -1) {
            perror("waitpid failed");
            return;
        }

        if (WIFEXITED(status)) {
            printf("[Parent Process] Child %d terminated normally with exit status: %d\n",
                   waited_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[Parent Process] Child %d terminated by signal: %d\n",
                   waited_pid, WTERMSIG(status));
        }
    }
}

void demo_execvp(void) {
    printf("\n=== 2. Process Replacement using execvp() ===\n");
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    } else if (pid == 0) {
        printf("  [Child Process] About to execute 'uname -a' using execvp()...\n");
        char *args[] = {"uname", "-a", NULL};
        execvp(args[0], args);

        /* execvp only returns if an error occurred */
        perror("execvp failed");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        printf("[Parent Process] Child completed execvp execution with status %d.\n",
               WEXITSTATUS(status));
    }
}

void demo_process_tree(void) {
    printf("\n=== 3. Process Tree Inspection Demo ===\n");
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    } else if (pid == 0) {
        printf("  [Child Process] PID=%d, sleeping 1s so parent can inspect /proc...\n", getpid());
        sleep(1);
        exit(0);
    } else {
        printf("[Parent Process] Inspecting Child PID %d in /proc:\n", pid);
        inspect_process_proc(pid);
        int status;
        waitpid(pid, &status, 0);
        printf("[Parent Process] Child process reaped successfully.\n");
    }
}

int main(int argc, char *argv[]) {
    printf("====================================================\n");
    printf("  OSSP Skill 01: Process Abstraction & Syscalls Demo\n");
    printf("====================================================\n");
    printf("Current PID: %d, Parent PID: %d\n", getpid(), getppid());

    if (argc > 1 && strcmp(argv[1], "--all") == 0) {
        demo_fork_and_relationships();
        demo_execvp();
        demo_process_tree();
        return 0;
    }

    int choice = 0;
    while (1) {
        printf("\nSelect an option to test:\n");
        printf("  1. Fork & Parent-Child Relationship\n");
        printf("  2. Process Replacement with execvp()\n");
        printf("  3. Process Tree & /proc Inspection\n");
        printf("  4. Run All Demos\n");
        printf("  5. Exit\n");
        printf("Choice [1-5]: ");
        fflush(stdout);

        if (scanf("%d", &choice) != 1) {
            break;
        }

        switch (choice) {
            case 1:
                demo_fork_and_relationships();
                break;
            case 2:
                demo_execvp();
                break;
            case 3:
                demo_process_tree();
                break;
            case 4:
                demo_fork_and_relationships();
                demo_execvp();
                demo_process_tree();
                break;
            case 5:
                printf("Exiting Skill 01.\n");
                return 0;
            default:
                printf("Invalid choice. Try again.\n");
        }
    }
    return 0;
}
