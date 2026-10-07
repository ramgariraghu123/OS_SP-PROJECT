#ifndef REDIR_H
#define REDIR_H

typedef struct {
    char **argv;
    int argc;
    char *input_file;
    char *output_file;
} RedirCommand;

RedirCommand *parse_redir_line(const char *line);
int execute_with_redirection(const RedirCommand *cmd);
void free_redir_command(RedirCommand *cmd);

#endif /* REDIR_H */
