#ifndef PIPELINE_H
#define PIPELINE_H

#define MAX_STAGES 8

typedef struct {
    char **argv;
    int argc;
    char *input_file;
    char *output_file;
    int append_mode;
} PipelineStage;

typedef struct {
    PipelineStage stages[MAX_STAGES];
    int stage_count;
    int is_background;
    char error_msg[256];
} Pipeline;

Pipeline *pipeline_create(const char *cmdline);
void pipeline_print_layout(const Pipeline *p);
int pipeline_validate(const Pipeline *p);
void pipeline_destroy(Pipeline *p);

#endif /* PIPELINE_H */
