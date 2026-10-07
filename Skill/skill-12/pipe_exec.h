#ifndef PIPE_EXEC_H
#define PIPE_EXEC_H

typedef struct {
    char **argv;
    int argc;
} CommandArgv;

/*
 * Executes N commands connected in a pipeline.
 * Returns the exit status of the final command in the pipeline.
 */
int execute_pipeline(CommandArgv *commands, int num_commands);

#endif /* PIPE_EXEC_H */
