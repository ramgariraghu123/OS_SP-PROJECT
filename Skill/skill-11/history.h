#ifndef HISTORY_H
#define HISTORY_H

#define MAX_HISTORY_CAPACITY 10

typedef struct {
    char *cmd;
    int entry_id;
} HistoryEntry;

typedef struct {
    HistoryEntry entries[MAX_HISTORY_CAPACITY];
    int count;      /* Current number of entries stored (<= capacity) */
    int total_added;/* Total commands added over shell lifetime */
} BoundedHistory;

BoundedHistory *history_init(void);
void history_push(BoundedHistory *h, const char *cmd);
void history_list(const BoundedHistory *h);
const char *history_get(const BoundedHistory *h, int entry_id);
int history_validate_consistency(const BoundedHistory *h);
void history_destroy(BoundedHistory *h);

#endif /* HISTORY_H */
