# Skill 13: Multi-Stage Pipelines

## Concepts Covered
- Setting up multi-stage pipelines with arbitrary $N$ processes.
- Managing an array of $N-1$ pipes (`int pipes[N-1][2]`).
- Connecting intermediate stages: stage $i$ reads from pipe $i-1$ and writes to pipe $i$.
- Closing all extraneous pipe file descriptors in children and parent to avoid hangs and deadlock.
- Reaping all children sequentially with `waitpid()`.
- Capturing final exit code of the pipeline.

## System Calls Used
- `pipe()`: Creates multiple anonymous communication channels.
- `dup2()`: Binds pipe read/write ends to standard input/output.
- `close()`: Closes file descriptors.
- `fork()`: Creates $N$ concurrent child processes.
- `execvp()`: Loads and runs commands.
- `waitpid()`: Blocks parent until each process in pipeline completes.

## Architecture & Flow Diagram
```
[Stage 0]               [Stage 1]               [Stage 2]
ls -1 /usr/include      grep stdio              sort -r
      |                      ^                      ^
      | stdout               | stdin                | stdin
      v                      |                      |
[pipe 0 write] ========> [pipe 0 read]              |
                             | stdout               |
                             v                      |
                         [pipe 1 write] ========> [pipe 1 read]
                                                       | stdout
                                                       v
                                                    Terminal
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o multi_pipe_exec
```

## How to Run
```bash
./multi_pipe_exec
```
