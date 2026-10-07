#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

typedef struct HistoryNode {
    char *cmd;
    struct HistoryNode *prev;
    struct HistoryNode *next;
} HistoryNode;

typedef struct HistoryList {
    HistoryNode *head;
    HistoryNode *tail;
    HistoryNode *nav_curr;
    size_t size;
} HistoryList;

HistoryList *history_create(void);
void history_add(HistoryList *list, const char *cmd);
const char *history_prev(HistoryList *list);
const char *history_next(HistoryList *list);
void history_reset_nav(HistoryList *list);
void history_free(HistoryList *list);

#endif /* HISTORY_H */
