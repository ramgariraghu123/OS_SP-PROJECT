#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pipeline.h"

static char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

Pipeline *pipeline_create(const char *cmdline) {
    Pipeline *p = (Pipeline *)calloc(1, sizeof(Pipeline));
    if (!cmdline) return p;

    char *copy = strdup(cmdline);
    char *trimmed = trim_whitespace(copy);

    /* Check background flag at end */
    size_t len = strlen(trimmed);
    if (len > 0 && trimmed[len - 1] == '&') {
        p->is_background = 1;
        trimmed[len - 1] = '\0';
        trimmed = trim_whitespace(trimmed);
    }

    /* Check trailing pipe */
    len = strlen(trimmed);
    if (len > 0 && trimmed[len - 1] == '|') {
        snprintf(p->error_msg, sizeof(p->error_msg), "Invalid pipeline layout: trailing '|'");
        free(copy);
        return p;
    }

    /* Split stages by '|' */
    char *saveptr;
    char *stage_str = strtok_r(trimmed, "|", &saveptr);
    while (stage_str) {
        if (p->stage_count >= MAX_STAGES) {
            snprintf(p->error_msg, sizeof(p->error_msg), "Pipeline exceeds maximum stage limit (%d)", MAX_STAGES);
            free(copy);
            return p;
        }

        char *clean_stage = trim_whitespace(stage_str);
        if (strlen(clean_stage) == 0) {
            snprintf(p->error_msg, sizeof(p->error_msg), "Invalid pipeline layout: empty stage between pipes");
            free(copy);
            return p;
        }

        PipelineStage *st = &p->stages[p->stage_count];

        /* Tokenize stage for args and redirections */
        char *sub_saveptr;
        char *tok = strtok_r(clean_stage, " \t", &sub_saveptr);
        while (tok) {
            if (strcmp(tok, "<") == 0) {
                char *target = strtok_r(NULL, " \t", &sub_saveptr);
                if (target) st->input_file = strdup(target);
            } else if (strcmp(tok, ">") == 0) {
                char *target = strtok_r(NULL, " \t", &sub_saveptr);
                if (target) {
                    st->output_file = strdup(target);
                    st->append_mode = 0;
                }
            } else if (strcmp(tok, ">>") == 0) {
                char *target = strtok_r(NULL, " \t", &sub_saveptr);
                if (target) {
                    st->output_file = strdup(target);
                    st->append_mode = 1;
                }
            } else {
                st->argv = (char **)realloc(st->argv, (st->argc + 2) * sizeof(char *));
                st->argv[st->argc++] = strdup(tok);
                st->argv[st->argc] = NULL;
            }
            tok = strtok_r(NULL, " \t", &sub_saveptr);
        }

        p->stage_count++;
        stage_str = strtok_r(NULL, "|", &saveptr);
    }

    free(copy);
    return p;
}

int pipeline_validate(const Pipeline *p) {
    if (!p) return 0;
    if (strlen(p->error_msg) > 0) return 0;
    if (p->stage_count == 0) return 0;

    for (int i = 0; i < p->stage_count; i++) {
        if (p->stages[i].argc == 0) return 0;
        /* Warning checks: stdin redir in middle stage */
        if (i > 0 && p->stages[i].input_file) {
            printf("  [Warning] Middle pipeline stage %d has input redirection '< %s'\n",
                   i, p->stages[i].input_file);
        }
        /* Warning checks: stdout redir in middle stage */
        if (i < p->stage_count - 1 && p->stages[i].output_file) {
            printf("  [Warning] Middle pipeline stage %d redirects output '> %s'\n",
                   i, p->stages[i].output_file);
        }
    }
    return 1;
}

void pipeline_print_layout(const Pipeline *p) {
    if (!p) return;
    if (strlen(p->error_msg) > 0) {
        printf("  [Layout Error] %s\n", p->error_msg);
        return;
    }

    printf("\n=== Pipeline Execution Topology ===\n");
    printf("Total Stages: %d | Background Mode: %s\n",
           p->stage_count, p->is_background ? "YES" : "NO");

    for (int i = 0; i < p->stage_count; i++) {
        printf("  Stage [%d]: ", i);
        for (int a = 0; a < p->stages[i].argc; a++) {
            printf("%s ", p->stages[i].argv[a]);
        }
        if (p->stages[i].input_file) {
            printf("< %s ", p->stages[i].input_file);
        }
        if (p->stages[i].output_file) {
            printf("%s %s ", p->stages[i].append_mode ? ">>" : ">", p->stages[i].output_file);
        }
        printf("\n");

        if (i < p->stage_count - 1) {
            printf("       │ (stdout / fd 1)\n");
            printf("       ▼\n");
            printf("    [Pipe %d: fd[%d][1] -> fd[%d][0]]\n", i, i, i);
            printf("       │ (stdin / fd 0)\n");
            printf("       ▼\n");
        }
    }
    printf("===================================\n\n");
}

void pipeline_destroy(Pipeline *p) {
    if (!p) return;
    for (int i = 0; i < p->stage_count; i++) {
        for (int a = 0; a < p->stages[i].argc; a++) {
            free(p->stages[i].argv[a]);
        }
        if (p->stages[i].argv) free(p->stages[i].argv);
        if (p->stages[i].input_file) free(p->stages[i].input_file);
        if (p->stages[i].output_file) free(p->stages[i].output_file);
    }
    free(p);
}
