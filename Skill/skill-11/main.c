#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"
#include "pipeline.h"

/*
 * OSSP Skill 11: Command History Structures & Pipeline Representation Structures.
 */

void test_history_overflow(BoundedHistory *h) {
    printf("\n--- Test 1: Bounded History Capacity Limits & Eviction ---\n");
    char cmd[32];
    for (int i = 1; i <= 14; i++) {
        snprintf(cmd, sizeof(cmd), "echo command_%d", i);
        history_push(h, cmd);
    }
    history_list(h);
    printf("Consistency Validation: %s\n",
           history_validate_consistency(h) ? "PASSED (Valid)" : "FAILED");

    printf("\nRecalling Command !12:\n");
    const char *recalled = history_get(h, 12);
    printf("  Result: \"%s\"\n", recalled ? recalled : "(not found)");
}

void test_pipeline_layouts(void) {
    printf("\n--- Test 2: Pipeline Layouts & Topology Structures ---\n");
    const char *p1 = "cat < names.txt | grep John | sort | uniq -c > output.txt &";
    printf("Analyzing: %s\n", p1);
    Pipeline *pipe1 = pipeline_create(p1);
    pipeline_validate(pipe1);
    pipeline_print_layout(pipe1);
    pipeline_destroy(pipe1);

    printf("--- Test 3: Pipeline Layout Error Detection ---\n");
    const char *p2 = "cat names.txt | | grep error";
    printf("Analyzing: %s\n", p2);
    Pipeline *pipe2 = pipeline_create(p2);
    pipeline_print_layout(pipe2);
    pipeline_destroy(pipe2);

    const char *p3 = "ls -l |";
    printf("Analyzing: %s\n", p3);
    Pipeline *pipe3 = pipeline_create(p3);
    pipeline_print_layout(pipe3);
    pipeline_destroy(pipe3);
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("  OSSP Skill 11: Bounded History & Pipeline Structures\n");
    printf("============================================================\n");

    BoundedHistory *hist = history_init();

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        test_history_overflow(hist);
        test_pipeline_layouts();
        history_destroy(hist);
        return 0;
    }

    char buffer[1024];
    while (1) {
        printf("struct-shell> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) break;
        if (strlen(buffer) == 0) continue;

        if (strcmp(buffer, "history") == 0) {
            history_list(hist);
            continue;
        }

        if (buffer[0] == '!') {
            int target_id = atoi(buffer + 1);
            const char *recall = history_get(hist, target_id);
            if (recall) {
                printf("  [Recall !%d]: %s\n", target_id, recall);
                strncpy(buffer, recall, sizeof(buffer) - 1);
            } else {
                printf("  [Error] No history entry !%d\n", target_id);
                continue;
            }
        }

        history_push(hist, buffer);

        Pipeline *p = pipeline_create(buffer);
        if (pipeline_validate(p)) {
            pipeline_print_layout(p);
        } else if (strlen(p->error_msg) > 0) {
            printf("  [Error] %s\n", p->error_msg);
        }
        pipeline_destroy(p);
    }

    history_destroy(hist);
    printf("Exiting Skill 11.\n");
    return 0;
}
