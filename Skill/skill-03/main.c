#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "history.h"

/*
 * OSSP Skill 03: Escape Sequences, Command History Navigation, Dynamic Buffers,
 * Resizing, Linked Lists, and Valgrind Verification.
 */

static struct termios orig_termios;
static int raw_active = 0;

void disable_raw(void) {
    if (raw_active) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_active = 0;
    }
}

int enable_raw(void) {
    if (!isatty(STDIN_FILENO)) return 0;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return 0;

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return 0;
    raw_active = 1;
    atexit(disable_raw);
    return 1;
}

/* Dynamic Buffer Structure */
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} DynamicBuffer;

DynamicBuffer *buffer_create(size_t initial_cap) {
    DynamicBuffer *buf = (DynamicBuffer *)malloc(sizeof(DynamicBuffer));
    if (!buf) return NULL;
    buf->capacity = initial_cap < 8 ? 8 : initial_cap;
    buf->data = (char *)malloc(buf->capacity);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    buf->data[0] = '\0';
    buf->length = 0;
    return buf;
}

void buffer_ensure_capacity(DynamicBuffer *buf, size_t needed) {
    if (needed >= buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        while (new_cap <= needed) new_cap *= 2;
        char *new_data = (char *)realloc(buf->data, new_cap);
        if (new_data) {
            buf->data = new_data;
            buf->capacity = new_cap;
        }
    }
}

void buffer_set(DynamicBuffer *buf, const char *str) {
    size_t len = strlen(str);
    buffer_ensure_capacity(buf, len + 1);
    strcpy(buf->data, str);
    buf->length = len;
}

void buffer_append_char(DynamicBuffer *buf, char c) {
    buffer_ensure_capacity(buf, buf->length + 2);
    buf->data[buf->length++] = c;
    buf->data[buf->length] = '\0';
}

void buffer_backspace(DynamicBuffer *buf) {
    if (buf->length > 0) {
        buf->length--;
        buf->data[buf->length] = '\0';
    }
}

void buffer_free(DynamicBuffer *buf) {
    if (buf) {
        free(buf->data);
        free(buf);
    }
}

int read_line_with_history(DynamicBuffer *buf, HistoryList *hist, const char *prompt) {
    buf->length = 0;
    buf->data[0] = '\0';

    if (!isatty(STDIN_FILENO)) {
        char temp[512];
        if (!fgets(temp, sizeof(temp), stdin)) return -1;
        size_t l = strlen(temp);
        if (l > 0 && temp[l - 1] == '\n') temp[l - 1] = '\0';
        buffer_set(buf, temp);
        return (int)buf->length;
    }

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) return -1;

        if (c == 4) { /* Ctrl+D */
            if (buf->length == 0) return -1;
            continue;
        }

        if (c == '\n' || c == '\r') {
            printf("\r\n");
            return (int)buf->length;
        }

        if (c == 127 || c == '\b') {
            if (buf->length > 0) {
                buffer_backspace(buf);
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        /* Check for ANSI Escape Sequences (Arrow Keys) */
        if (c == '\033') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;

            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    /* Up Arrow: Previous Command */
                    const char *prev_cmd = history_prev(hist);
                    if (prev_cmd) {
                        buffer_set(buf, prev_cmd);
                        /* Erase current terminal line and redraw */
                        printf("\r\033[2K%s%s", prompt, buf->data);
                        fflush(stdout);
                    }
                } else if (seq[1] == 'B') {
                    /* Down Arrow: Next Command */
                    const char *next_cmd = history_next(hist);
                    if (next_cmd) {
                        buffer_set(buf, next_cmd);
                        printf("\r\033[2K%s%s", prompt, buf->data);
                        fflush(stdout);
                    }
                }
            }
            continue;
        }

        /* Normal character: dynamically resize and append */
        if (c >= 32 && c <= 126) {
            buffer_append_char(buf, c);
            putchar(c);
            fflush(stdout);
        }
    }
}

int main(void) {
    const char *prompt = "myshell-03> ";
    printf("============================================================\n");
    printf("  OSSP Skill 03: Dynamic Buffers & Command History Navigation\n");
    printf("  Use UP/DOWN arrow keys to recall history. Type 'exit' to quit.\n");
    printf("============================================================\n");

    enable_raw();

    HistoryList *hist = history_create();
    DynamicBuffer *buf = buffer_create(8); /* Starts with small capacity 8 bytes to test dynamic resizing */

    while (1) {
        printf("%s", prompt);
        fflush(stdout);

        int len = read_line_with_history(buf, hist, prompt);
        if (len < 0) {
            printf("[Shell] EOF detected. Exiting.\n");
            break;
        }

        char *cmd = buf->data;
        while (*cmd == ' ' || *cmd == '\t') cmd++;

        if (strlen(cmd) == 0) continue;

        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            printf("[Shell] Exiting.\n");
            break;
        }

        /* Add to history */
        history_add(hist, cmd);

        printf("  [Executed] \"%s\" (Buffer capacity: %zu bytes, Length: %zu)\n",
               cmd, buf->capacity, buf->length);
    }

    /* Free all allocated memory cleanly */
    buffer_free(buf);
    history_free(hist);
    disable_raw();
    return 0;
}
