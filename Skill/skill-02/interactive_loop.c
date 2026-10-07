/**
 * Skill 02: Interactive Shell Loop and Keyboard Input Handling
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Shell Read-Eval-Print Loop (REPL) architecture
 * - Dynamic prompt rendering with working directory
 * - Low-level keyboard input processing (Backspace, Enter, Carriage Return)
 * - Input buffer management and overflow prevention
 * - Handling exit conditions and EOF (Ctrl+D)
 * - Interactive and automated self-test modes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>

#define MAX_INPUT_BUFFER 1024
#define PROMPT_STR "ossp-shell$ "

// Process an entered line in the REPL
int process_command(char *line) {
    // Strip leading whitespace
    while (*line == ' ' || *line == '\t') line++;

    // Strip trailing newline / carriage return
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ')) {
        line[--len] = '\0';
    }

    if (len == 0) {
        return 0; // Empty command
    }

    if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
        printf("[Shell] Exit command received. Terminating interactive loop.\n");
        return -1; // Signals exit
    } else if (strcmp(line, "help") == 0) {
        printf("Available built-in commands:\n");
        printf("  help     - Show this help message\n");
        printf("  status   - Show shell session status\n");
        printf("  echo ... - Print text to console\n");
        printf("  exit     - Exit the shell session\n");
    } else if (strcmp(line, "status") == 0) {
        printf("[Shell Status] PID: %d, PPID: %d, TTY: %s\n",
               getpid(), getppid(), isatty(STDIN_FILENO) ? "Yes" : "No (Pipe/File)");
    } else if (strncmp(line, "echo ", 5) == 0) {
        printf("%s\n", line + 5);
    } else {
        printf("[Shell] Unknown command: '%s'. Type 'help' for available commands.\n", line);
    }

    return 0;
}

// Low-level buffer input reader demonstrating backspace & enter handling
int read_input_line(char *buffer, size_t max_len) {
    size_t pos = 0;
    int ch;

    // Check if terminal is interactive
    bool is_terminal = isatty(STDIN_FILENO);

    if (!is_terminal) {
        // Non-interactive / piped input
        if (fgets(buffer, (int)max_len, stdin) == NULL) {
            return -1; // EOF
        }
        return (int)strlen(buffer);
    }

    // Set non-canonical mode to demonstrate character-by-character backspace handling
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // Raw character reading without automatic echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (1) {
        ch = getchar();
        if (ch == EOF) {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return -1;
        }

        if (ch == '\n' || ch == '\r') {
            // Enter key pressed
            putchar('\n');
            buffer[pos] = '\0';
            break;
        } else if (ch == 127 || ch == '\b') {
            // Backspace handling
            if (pos > 0) {
                pos--;
                // Visual backspace: move back, erase char with space, move back again
                printf("\b \b");
                fflush(stdout);
            }
        } else if (ch == 4) {
            // Ctrl+D (EOT)
            if (pos == 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                return -1;
            }
        } else {
            // Regular character
            if (pos < max_len - 1) {
                buffer[pos++] = (char)ch;
                putchar(ch);
                fflush(stdout);
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return (int)pos;
}

void run_automated_demo() {
    printf("[Skill 02] Running automated demonstration mode...\n");
    char test_inputs[][64] = {
        "status",
        "echo Systems Programming Shell REPL Active",
        "help",
        "exit"
    };

    int num_tests = sizeof(test_inputs) / sizeof(test_inputs[0]);
    for (int i = 0; i < num_tests; i++) {
        printf("%s%s\n", PROMPT_STR, test_inputs[i]);
        if (process_command(test_inputs[i]) < 0) {
            printf("[Shell] Clean termination reached in demo.\n");
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    printf("[Skill 02] Interactive Shell Loop and Input Processing\n");

    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        run_automated_demo();
        return EXIT_SUCCESS;
    }

    // If stdin is not a TTY (e.g. running from make test or automated pipe)
    if (!isatty(STDIN_FILENO)) {
        printf("[Shell] Running non-interactive stream input mode.\n");
        char buffer[MAX_INPUT_BUFFER];
        while (1) {
            printf("%s", PROMPT_STR);
            fflush(stdout);
            if (read_input_line(buffer, sizeof(buffer)) < 0) {
                printf("\n[Shell] End of input stream reached (EOF).\n");
                break;
            }
            if (process_command(buffer) < 0) {
                break;
            }
        }
        return EXIT_SUCCESS;
    }

    // Interactive Loop
    printf("[Shell] Interactive session started. Type 'help' or 'exit'. (Ctrl+D to exit)\n");
    char buffer[MAX_INPUT_BUFFER];
    while (1) {
        printf("%s", PROMPT_STR);
        fflush(stdout);

        int bytes_read = read_input_line(buffer, sizeof(buffer));
        if (bytes_read < 0) {
            printf("\n[Shell] Received EOF (Ctrl+D). Goodbye!\n");
            break;
        }

        if (process_command(buffer) < 0) {
            break;
        }
    }

    printf("[Skill 02] Session finished successfully.\n");
    return EXIT_SUCCESS;
}
