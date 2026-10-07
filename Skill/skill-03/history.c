#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

HistoryList *history_create(void) {
    HistoryList *list = (HistoryList *)malloc(sizeof(HistoryList));
    if (!list) return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->nav_curr = NULL;
    list->size = 0;
    return list;
}

void history_add(HistoryList *list, const char *cmd) {
    if (!list || !cmd || strlen(cmd) == 0) return;

    /* Don't add duplicate if same as the most recent command */
    if (list->tail && strcmp(list->tail->cmd, cmd) == 0) {
        history_reset_nav(list);
        return;
    }

    HistoryNode *node = (HistoryNode *)malloc(sizeof(HistoryNode));
    if (!node) return;

    node->cmd = strdup(cmd);
    node->next = NULL;
    node->prev = list->tail;

    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    list->size++;
    history_reset_nav(list);
}

const char *history_prev(HistoryList *list) {
    if (!list || !list->head) return NULL;

    if (list->nav_curr == NULL) {
        /* Start from most recent (tail) */
        list->nav_curr = list->tail;
    } else if (list->nav_curr->prev != NULL) {
        list->nav_curr = list->nav_curr->prev;
    }
    return list->nav_curr ? list->nav_curr->cmd : NULL;
}

const char *history_next(HistoryList *list) {
    if (!list || !list->head || !list->nav_curr) return NULL;

    list->nav_curr = list->nav_curr->next;
    if (list->nav_curr) {
        return list->nav_curr->cmd;
    }
    return ""; /* Reached past the newest command (blank prompt) */
}

void history_reset_nav(HistoryList *list) {
    if (list) {
        list->nav_curr = NULL;
    }
}

void history_free(HistoryList *list) {
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
