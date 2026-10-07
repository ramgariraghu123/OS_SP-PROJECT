/**
 * Skill 03: Dynamic Buffer Management and History Navigation
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Dynamic buffer allocation (malloc, realloc) with automatic capacity doubling
 * - Overflow prevention with boundary checks
 * - Doubly linked list for command history management
 * - In-memory history navigation (Previous/Next recall)
 * - ANSI Escape sequence parsing (Up Arrow \033[A, Down Arrow \033[B)
 * - Complete memory lifecycle cleanup (free)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>

#define INITIAL_BUF_CAP 8

/* --- Dynamic Buffer Management --- */
typedef struct {
    char *data;
    size_t capacity;
    size_t length;
} DynBuffer;

DynBuffer *buffer_create(size_t initial_cap) {
    DynBuffer *buf = (DynBuffer *)malloc(sizeof(DynBuffer));
    if (!buf) {
        perror("malloc buffer struct failed");
        exit(EXIT_FAILURE);
    }
    buf->capacity = initial_cap ? initial_cap : INITIAL_BUF_CAP;
    buf->data = (char *)malloc(buf->capacity);
    if (!buf->data) {
        perror("malloc buffer data failed");
        free(buf);
        exit(EXIT_FAILURE);
    }
    buf->data[0] = '\0';
    buf->length = 0;
    return buf;
}

void buffer_clear(DynBuffer *buf) {
    buf->length = 0;
    buf->data[0] = '\0';
}

void buffer_append_char(DynBuffer *buf, char c) {
    if (buf->length + 2 > buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        char *new_data = (char *)realloc(buf->data, new_cap);
        if (!new_data) {
            perror("realloc failed");
            return;
        }
        buf->data = new_data;
        buf->capacity = new_cap;
    }
    buf->data[buf->length++] = c;
    buf->data[buf->length] = '\0';
}

void buffer_set_text(DynBuffer *buf, const char *text) {
    size_t new_len = strlen(text);
    while (new_len + 1 > buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        char *new_data = (char *)realloc(buf->data, new_cap);
        if (!new_data) {
            perror("realloc failed");
            return;
        }
        buf->data = new_data;
        buf->capacity = new_cap;
    }
    memcpy(buf->data, text, new_len + 1);
    buf->length = new_len;
}

void buffer_pop_char(DynBuffer *buf) {
    if (buf->length > 0) {
        buf->length--;
        buf->data[buf->length] = '\0';
    }
}

void buffer_destroy(DynBuffer *buf) {
    if (buf) {
        if (buf->data) free(buf->data);
        free(buf);
    }
}

/* --- History Doubly Linked List --- */
typedef struct HistoryNode {
    int id;
    char *cmd;
    struct HistoryNode *prev;
    struct HistoryNode *next;
} HistoryNode;

typedef struct {
    HistoryNode *head;
    HistoryNode *tail;
    int count;
} HistoryList;

HistoryList *history_create() {
    HistoryList *list = (HistoryList *)malloc(sizeof(HistoryList));
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    return list;
}

void history_add(HistoryList *list, const char *cmd) {
    if (!cmd || strlen(cmd) == 0) return;

    HistoryNode *node = (HistoryNode *)malloc(sizeof(HistoryNode));
    node->id = ++list->count;
    node->cmd = strdup(cmd);
    node->next = NULL;
    node->prev = list->tail;

    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
}

void history_display(const HistoryList *list) {
    printf("\n--- Command History (%d entries) ---\n", list->count);
    HistoryNode *curr = list->head;
    while (curr) {
        printf("  [%3d] %s\n", curr->id, curr->cmd);
        curr = curr->next;
    }
    printf("------------------------------------\n");
}

void history_destroy(HistoryList *list) {
    if (!list) return;
    HistoryNode *curr = list->head;
    while (curr) {
        HistoryNode *next = curr->next;
        free(curr->cmd);
        free(curr);
        curr = next;
    }
    free(list);
}

/* --- Demo & Interactive Execution --- */
void run_demo() {
    printf("[Skill 03] Demonstrating dynamic buffer growth and history list...\n");

    DynBuffer *buf = buffer_create(4); // Start intentionally tiny (4 bytes)
    printf("Initial buffer capacity: %zu bytes\n", buf->capacity);

    const char *test_phrase = "Operating Systems and Systems Programming (OSSP) dynamic buffer stress test string!";
    for (size_t i = 0; i < strlen(test_phrase); i++) {
        buffer_append_char(buf, test_phrase[i]);
    }

    printf("Expanded buffer content: '%s'\n", buf->data);
    printf("Final buffer length: %zu, capacity: %zu bytes (resized safely without overflow)\n",
           buf->length, buf->capacity);

    // Test History List
    HistoryList *hist = history_create();
    history_add(hist, "ls -la");
    history_add(hist, "cat /etc/os-release");
    history_add(hist, "grep -r 'main' .");
    history_add(hist, "gcc -o app main.c");

    history_display(hist);

    // Simulate navigation
    HistoryNode *nav = hist->tail;
    printf("\nSimulating Up Arrow navigation (Backwards in time):\n");
    while (nav) {
        printf("  [Recall Up] -> %s\n", nav->cmd);
        nav = nav->prev;
    }

    printf("\nSimulating Down Arrow navigation (Forwards in time):\n");
    nav = hist->head;
    while (nav) {
        printf("  [Recall Down] -> %s\n", nav->cmd);
        nav = nav->next;
    }

    // Clean up memory
    buffer_destroy(buf);
    history_destroy(hist);
    printf("\n[Skill 03] All dynamic memory buffers and history nodes successfully freed.\n");
}

int main(int argc, char *argv[]) {
    printf("[Skill 03] Dynamic Buffer Management & History Navigation\n");

    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        run_demo();
        return EXIT_SUCCESS;
    }

    // If stdin is not a terminal, run demo
    if (!isatty(STDIN_FILENO)) {
        run_demo();
        return EXIT_SUCCESS;
    }

    // Terminal interactive session
    DynBuffer *buf = buffer_create(16);
    HistoryList *hist = history_create();
    HistoryNode *cursor = NULL;

    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("Interactive shell with Dynamic Buffer and Up/Down arrow history navigation.\n");
    printf("Commands: 'history', 'exit'. Press UP/DOWN to navigate past commands.\n");

    while (1) {
        printf("ossp$ %s", buf->data);
        fflush(stdout);

        int c = getchar();
        if (c == EOF || c == 4) break; // EOF or Ctrl+D

        if (c == '\033') { // Escape sequence
            int seq1 = getchar();
            if (seq1 == '[') {
                int seq2 = getchar();
                if (seq2 == 'A') { // Up arrow
                    if (cursor == NULL) {
                        cursor = hist->tail;
                    } else if (cursor->prev) {
                        cursor = cursor->prev;
                    }
                    if (cursor) {
                        // Erase current input on line
                        for (size_t k = 0; k < buf->length; k++) printf("\b \b");
                        buffer_set_text(buf, cursor->cmd);
                    }
                } else if (seq2 == 'B') { // Down arrow
                    if (cursor && cursor->next) {
                        cursor = cursor->next;
                        for (size_t k = 0; k < buf->length; k++) printf("\b \b");
                        buffer_set_text(buf, cursor->cmd);
                    } else if (cursor && cursor->next == NULL) {
                        cursor = NULL;
                        for (size_t k = 0; k < buf->length; k++) printf("\b \b");
                        buffer_clear(buf);
                    }
                }
            }
            continue;
        }

        if (c == '\n' || c == '\r') {
            putchar('\n');
            if (buf->length > 0) {
                if (strcmp(buf->data, "exit") == 0) break;
                if (strcmp(buf->data, "history") == 0) {
                    history_display(hist);
                } else {
                    printf("Executed: %s\n", buf->data);
                    history_add(hist, buf->data);
                }
                buffer_clear(buf);
                cursor = NULL;
            }
            continue;
        }

        if (c == 127 || c == '\b') {
            if (buf->length > 0) {
                buffer_pop_char(buf);
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        // Normal char
        buffer_append_char(buf, (char)c);
        putchar(c);
        fflush(stdout);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    buffer_destroy(buf);
    history_destroy(hist);
    printf("\nExited cleanly.\n");
    return EXIT_SUCCESS;
}
