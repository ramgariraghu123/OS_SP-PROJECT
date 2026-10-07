#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

BoundedHistory *history_init(void) {
    BoundedHistory *h = (BoundedHistory *)calloc(1, sizeof(BoundedHistory));
    return h;
}

void history_push(BoundedHistory *h, const char *cmd) {
    if (!h || !cmd || strlen(cmd) == 0) return;

    h->total_added++;

    if (h->count < MAX_HISTORY_CAPACITY) {
        /* Buffer not yet full */
        h->entries[h->count].cmd = strdup(cmd);
        h->entries[h->count].entry_id = h->total_added;
        h->count++;
    } else {
        /* Buffer full: evict oldest entry at index 0 and shift left */
        free(h->entries[0].cmd);
        for (int i = 0; i < MAX_HISTORY_CAPACITY - 1; i++) {
            h->entries[i] = h->entries[i + 1];
        }
        h->entries[MAX_HISTORY_CAPACITY - 1].cmd = strdup(cmd);
        h->entries[MAX_HISTORY_CAPACITY - 1].entry_id = h->total_added;
    }
}

void history_list(const BoundedHistory *h) {
    if (!h || h->count == 0) {
        printf("  [History is empty]\n");
        return;
    }
    printf("  Index   CmdID   Command\n");
    printf("  ------------------------------------\n");
    for (int i = 0; i < h->count; i++) {
        printf("  [%2d]     !%-4d  %s\n", i + 1, h->entries[i].entry_id, h->entries[i].cmd);
    }
    printf("  ------------------------------------\n");
    printf("  Stored: %d / Capacity: %d (Total added: %d)\n",
           h->count, MAX_HISTORY_CAPACITY, h->total_added);
}

const char *history_get(const BoundedHistory *h, int entry_id) {
    if (!h) return NULL;
    for (int i = 0; i < h->count; i++) {
        if (h->entries[i].entry_id == entry_id) {
            return h->entries[i].cmd;
        }
    }
    return NULL;
}

int history_validate_consistency(const BoundedHistory *h) {
    if (!h) return 0;
    if (h->count < 0 || h->count > MAX_HISTORY_CAPACITY) return 0;
    for (int i = 0; i < h->count; i++) {
        if (h->entries[i].cmd == NULL) return 0;
        if (i > 0 && h->entries[i].entry_id <= h->entries[i - 1].entry_id) return 0;
    }
    return 1;
}

void history_destroy(BoundedHistory *h) {
    if (!h) return;
    for (int i = 0; i < h->count; i++) {
        if (h->entries[i].cmd) free(h->entries[i].cmd);
    }
    free(h);
}
