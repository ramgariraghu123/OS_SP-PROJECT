#ifndef COMPLEX_EXEC_H
#define COMPLEX_EXEC_H

typedef struct {
    char **argv;
    int argc;
    char *input_file;
    char *output_file;
    int append_mode;
    char *err_file;
    int merge_err_to_out; /* 1 if 2>&1 or &> */
} ComplexStage;

typedef struct {
    ComplexStage *stages;
    int num_stages;
} ComplexPipeline;

ComplexPipeline *parse_complex_command(const char *cmdline);
void print_execution_plan(const ComplexPipeline *cp);
int execute_complex_pipeline(const ComplexPipeline *cp);
void free_complex_pipeline(ComplexPipeline *cp);

#endif /* COMPLEX_EXEC_H */
