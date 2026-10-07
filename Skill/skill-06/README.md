# Skill 06: Escape Sequences & Process Execution

## Concepts Covered
- Parsing backslash escape characters (`\ ` for literal spaces, `\|`, `\<`, `\>`).
- Preventing premature token segmentation when escaping whitespace.
- Building standard null-terminated argument vectors (`char *argv[]`).
- Spawning child processes with `fork()`.
- Program execution via `execvp()` with error detection using `perror()` and `strerror()`.
- Parent process synchronization and exit code harvesting using `waitpid()`.

## System Calls Used
- `fork()`: Spawns child process.
- `execvp()`: Loads and runs program from argument vector.
- `waitpid()`: Blocks parent until child finishes; collects status flags.
- `getpid()`: Fetches process ID for logging and tracing.

## Architecture & Flow Diagram
```
Raw input with escapes:
"echo Hello\ World\|Test"
           |
           v
[Escape Sequence Parser]
           |
           v
argv = ["echo", "Hello World|Test", NULL]
           |
           v
        fork()
       /      \
      v        v
   [Parent]   [Child]
      |          |
      |          v
      |       execvp("echo", argv)
      |          |
      +<-- waitpid()
      |
   Exit status: 0
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o escape_exec
```

## How to Run
```bash
# Automated tests
./escape_exec

# Custom command with escapes
./escape_exec echo This\ is\ a\ single\ argument
```
