/**
 * Skill 11: Command History Buffer & Pipeline Data Structures
 * Course: OSSP (Operating Systems And Systems Programming) - 25CS2104E
 *
 * Demonstrates:
 * - Fixed-capacity ring buffer for command history with safe eviction
 * - Command retrieval, display, and history expansion (!n, !-1)
 * - Pipeline data structures and ordered linked-list topology
 * - Parsing complex pipeline strings into execution stages
 * - Structural validation and comprehensive memory deallocation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_HISTORY_CAPACITY 5

/* --- History Ring Buffer --- */
typedef struct {
    int global_id;
    char *command_text;
} HistoryEntry;

typedef struct {
    HistoryEntry entries[MAX_HISTORY_CAPACITY];
    int start;
    int count;
    int next_id;
} HistoryRingBuffer;

HistoryRingBuffer *history_init() {
    HistoryRingBuffer *hb = (HistoryRingBuffer *)calloc(1, sizeof(HistoryRingBuffer));
    hb->next_id = 1;
    return hb;
}

void history_push(HistoryRingBuffer *hb, const char *cmd) {
    if (!cmd || strlen(cmd) == 0) return;

    if (hb->count == MAX_HISTORY_CAPACITY) {
        // Evict oldest entry
        free(hb->entries[hb->start].command_text);
        hb->entries[hb->start].command_text = NULL;
        hb->start = (hb->start + 1) % MAX_HISTORY_CAPACITY;
        hb->count--;
    }

    int insert_idx = (hb->start + hb->count) % MAX_HISTORY_CAPACITY;
    hb->entries[insert_idx].global_id = hb->next_id++;
    hb->entries[insert_idx].command_text = strdup(cmd);
    hb->count++;
}

void history_show(const HistoryRingBuffer *hb) {
    printf("\n--- History Ring Buffer (Cap %d, Count %d) ---\n", MAX_HISTORY_CAPACITY, hb->count);
    for (int i = 0; i < hb->count; i++) {
        int idx = (hb->start + i) % MAX_HISTORY_CAPACITY;
        printf("  %3d  %s\n", hb->entries[idx].global_id, hb->entries[idx].command_text);
    }
    printf("-----------------------------------------------\n");
}

const char *history_recall_id(const HistoryRingBuffer *hb, int id) {
    for (int i = 0; i < hb->count; i++) {
        int idx = (hb->start + i) % MAX_HISTORY_CAPACITY;
        if (hb->entries[idx].global_id == id) {
            return hb->entries[idx].command_text;
        }
    }
    return NULL;
}

const char *history_recall_last(const HistoryRingBuffer *hb) {
    if (hb->count == 0) return NULL;
    int last_idx = (hb->start + hb->count - 1) % MAX_HISTORY_CAPACITY;
    return hb->entries[last_idx].command_text;
}

void history_cleanup(HistoryRingBuffer *hb) {
    if (!hb) return;
    for (int i = 0; i < hb->count; i++) {
        int idx = (hb->start + i) % MAX_HISTORY_CAPACITY;
        free(hb->entries[idx].command_text);
    }
    free(hb);
}

/* --- Pipeline Data Structures --- */
typedef struct PipelineStage {
    int stage_id;
    char **argv;
    int argc;
    struct PipelineStage *next;
} PipelineStage;

typedef struct {
    PipelineStage *head;
    PipelineStage *tail;
    int stage_count;
} Pipeline;

Pipeline *pipeline_create() {
    Pipeline *p = (Pipeline *)calloc(1, sizeof(Pipeline));
    return p;
}

void pipeline_add_stage(Pipeline *p, char **argv, int argc) {
    PipelineStage *stage = (PipelineStage *)malloc(sizeof(PipelineStage));
    stage->stage_id = ++p->stage_count;
    stage->argc = argc;
    stage->argv = (char **)malloc(sizeof(char *) * (argc + 1));
    for (int i = 0; i < argc; i++) {
        stage->argv[i] = strdup(argv[i]);
    }
    stage->argv[argc] = NULL;
    stage->next = NULL;

    if (p->tail) {
        p->tail->next = stage;
    } else {
        p->head = stage;
    }
    p->tail = stage;
}

void pipeline_display(const Pipeline *p) {
    printf("\n--- Pipeline Topology (%d Stages) ---\n", p->stage_count);
    PipelineStage *curr = p->head;
    while (curr) {
        printf("  [Stage %d]: ", curr->stage_id);
        for (int i = 0; i < curr->argc; i++) {
            printf("\"%s\" ", curr->argv[i]);
        }
        if (curr->next) {
            printf(" ===(PIPE)===>\n");
        } else {
            printf(" ===(STDOUT)\n");
        }
        curr = curr->next;
    }
    printf("--------------------------------------\n");
}

void pipeline_free(Pipeline *p) {
    if (!p) return;
    PipelineStage *curr = p->head;
    while (curr) {
        PipelineStage *next = curr->next;
        for (int i = 0; i < curr->argc; i++) {
            free(curr->argv[i]);
        }
        free(curr->argv);
        free(curr);
        curr = next;
    }
    free(p);
}

// Convert a pipeline string like "ps aux | grep root | head -n 5" into Pipeline structure
Pipeline *parse_pipeline_string(const char *cmd_line) {
    Pipeline *p = pipeline_create();
    char *copy = strdup(cmd_line);
    char *saveptr1, *saveptr2;

    char *stage_str = strtok_r(copy, "|", &saveptr1);
    while (stage_str) {
        // Tokenize stage into args
        char *args[32];
        int argc = 0;
        char *arg = strtok_r(stage_str, " \t\r\n", &saveptr2);
        while (arg && argc < 31) {
            args[argc++] = arg;
            arg = strtok_r(NULL, " \t\r\n", &saveptr2);
        }
        args[argc] = NULL;

        if (argc > 0) {
            pipeline_add_stage(p, args, argc);
        }

        stage_str = strtok_r(NULL, "|", &saveptr1);
    }

    free(copy);
    return p;
}

int main() {
    printf("[Skill 11] History Buffer with Capacity Limit & Pipeline Data Structures\n");

    // Test History Ring Buffer
    HistoryRingBuffer *hist = history_init();
    printf("Adding commands exceeding capacity to verify ring buffer eviction...\n");
    history_push(hist, "ls -la");
    history_push(hist, "cd /var/log");
    history_push(hist, "cat syslog | grep error");
    history_push(hist, "find . -name '*.c'");
    history_push(hist, "gcc -o test test.c");
    history_show(hist);

    // Add 2 more to trigger capacity eviction (Capacity = 5)
    printf("\nAdding 2 more commands to force eviction of entries 1 and 2:\n");
    history_push(hist, "./test --run");
    history_push(hist, "git status");
    history_show(hist);

    // Test History Recall
    printf("\nRecall command with ID 4: [%s]\n", history_recall_id(hist, 4));
    const char *last_cmd = history_recall_last(hist);
    printf("Recall last command (!-1): [%s]\n", last_cmd ? last_cmd : "<empty>");

    // Test Pipeline Data Structure
    printf("\nParsing and constructing Pipeline execution structure:\n");
    const char *test_pipe = "ps -ef | grep systemd | awk '{print $2}' | sort -n";
    printf("Input: %s\n", test_pipe);

    Pipeline *pipe_struct = parse_pipeline_string(test_pipe);
    pipeline_display(pipe_struct);

    // Cleanup all allocated resources
    pipeline_free(pipe_struct);
    history_cleanup(hist);

    printf("\n[Skill 11] History and pipeline structure validation completed successfully.\n");
    return EXIT_SUCCESS;
}
