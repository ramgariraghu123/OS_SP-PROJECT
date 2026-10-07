#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_BUFFER_SIZE 1024

/*
 * OSSP Skill 02: Interactive Shell Main Loop, Input Buffer, Backspace Handling,
 * Enter Processing, and Exit Conditions.
 *
 * System calls & APIs demonstrated:
 *   - isatty(): checks if standard input is a terminal
 *   - tcgetattr(): gets terminal attributes
 *   - tcsetattr(): sets terminal attributes for raw/non-canonical keyboard capture
 *   - read() / write(): low-level character I/O
 */

static struct termios orig_termios;
static int raw_mode_active = 0;

void disable_raw_mode(void) {
    if (raw_mode_active) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_active = 0;
    }
}

int enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO)) {
        return 0; /* Not a terminal (e.g. piped input), don't set raw mode */
    }
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        return 0;
    }
    struct termios raw = orig_termios;
    /* Disable canonical mode (ICANON) and echo (ECHO) so we process every key */
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return 0;
    }
    raw_mode_active = 1;
    atexit(disable_raw_mode);
    return 1;
}

/*
 * Reads a line from the user.
 * In terminal mode: processes characters one by one, handling Backspace, Enter, and Ctrl+D.
 * In non-terminal mode: uses standard line reading.
 */
int read_input_line(char *buffer, size_t max_len) {
    size_t pos = 0;
    buffer[0] = '\0';

    if (!isatty(STDIN_FILENO)) {
        /* Piped / batch mode */
        if (!fgets(buffer, (int)max_len, stdin)) {
            return -1; /* EOF */
        }
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        return (int)strlen(buffer);
    }

    /* Terminal interactive raw mode */
    while (1) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n <= 0) {
            return -1; /* Error or EOF */
        }

        /* Handle EOF (Ctrl+D) */
        if (c == 4) { /* ASCII 4 is EOT (Ctrl+D) */
            if (pos == 0) {
                printf("\n");
                return -1; /* Exit on empty line with Ctrl+D */
            }
            continue;
        }

        /* Handle Enter key (\r or \n) */
        if (c == '\n' || c == '\r') {
            buffer[pos] = '\0';
            printf("\r\n");
            return (int)pos;
        }

        /* Handle Backspace key (ASCII 127 DEL or \b) */
        if (c == 127 || c == '\b') {
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                /* Erase character visually on terminal: back, space, back */
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        /* Handle normal printable characters */
        if (c >= 32 && c <= 126) {
            if (pos < max_len - 1) {
                buffer[pos++] = c;
                buffer[pos] = '\0';
                putchar(c);
                fflush(stdout);
            } else {
                /* Buffer full alert bell */
                putchar('\a');
                fflush(stdout);
            }
            continue;
        }
    }
}

int main(void) {
    char input_buffer[MAX_BUFFER_SIZE];
    const char *prompt = "myshell-02> ";

    printf("============================================================\n");
    printf("  OSSP Skill 02: Interactive Shell Main Loop & Key Capture\n");
    printf("  Commands: type anything, test Backspace, Enter, 'exit'/'quit'\n");
    printf("============================================================\n");

    int raw_enabled = enable_raw_mode();
    if (!raw_enabled && isatty(STDIN_FILENO)) {
        fprintf(stderr, "Warning: Could not enable raw terminal mode.\n");
    }

    while (1) {
        printf("%s", prompt);
        fflush(stdout);

        int len = read_input_line(input_buffer, sizeof(input_buffer));

        if (len < 0) {
            printf("[Shell] Detected EOF (Ctrl+D). Exiting.\n");
            break;
        }

        /* Trim leading whitespace */
        char *cmd = input_buffer;
        while (*cmd == ' ' || *cmd == '\t') cmd++;

        if (strlen(cmd) == 0) {
            continue; /* Empty line, re-prompt */
        }

        /* Check exit conditions */
        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            printf("[Shell] Exit condition matched ('%s'). Exiting cleanly.\n", cmd);
            break;
        }

        /* Process and demonstrate input buffer management */
        printf("  [Input Buffer Received] Length: %zu chars | Content: \"%s\"\n",
               strlen(cmd), cmd);
    }

    disable_raw_mode();
    return 0;
}
