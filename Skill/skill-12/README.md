# Skill 12: Single Pipe Execution (IPC)

## Concepts Covered
- Unidirectional inter-process communication using `pipe()`.
- Duplicating file descriptors to `STDIN_FILENO` and `STDOUT_FILENO` using `dup2()`.
- Critical descriptor hygiene: closing unused read and write ends in parent and children to ensure EOF is signaled.
- Spawning coordinated pairs of child processes.
- Synchronizing multiple children using sequential `waitpid()` calls.

## System Calls Used
- `pipe()`: Creates an anonymous unidirectional data channel (returns two file descriptors).
- `dup2()`: Clones an open file descriptor onto standard input/output.
- `close()`: Closes open file descriptors.
- `fork()`: Creates child worker processes.
- `execvp()`: Executes programs in child context.
- `waitpid()`: Waits for both child processes to exit.

## Architecture & Flow Diagram
```
                       pipefd[0] (read) <--- pipefd[1] (write)
                              ^                     ^
                              |                     |
                        dup2(..., STDIN)      dup2(..., STDOUT)
                              |                     |
                     +--------+-------+    +--------+-------+
                     | Child 2 Reader |    | Child 1 Writer |
                     | (e.g. tr A-Z)  |    | (e.g. echo)    |
                     +----------------+    +----------------+
                                       \  /
                                     fork()
                                       |
                                [Parent Shell]
                         (Closes pipefd[0] & pipefd[1],
                          Calls waitpid on both children)
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o single_pipe_exec
```

## How to Run
```bash
# Run automated tests
./single_pipe_exec

# Run custom pipeline
./single_pipe_exec ls \| wc -l
```
