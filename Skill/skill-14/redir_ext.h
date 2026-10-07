#ifndef REDIR_EXT_H
#define REDIR_EXT_H

typedef struct {
    char **argv;
    int argc;
    char *append_out_file; /* for >> */
    char *err_file;        /* for 2> */
} RedirExtCommand;

RedirExtCommand *parse_redir_ext_line(const char *line);
int execute_redir_ext(const RedirExtCommand *cmd);
void free_redir_ext(RedirExtCommand *cmd);

#endif /* REDIR_EXT_H */
