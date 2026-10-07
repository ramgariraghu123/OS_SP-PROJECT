# Skill 01: Process Abstraction and Hierarchy

## Concepts Covered
- Linux process abstraction and process lifecycle.
- Creating child processes using `fork()`.
- Inspecting Process IDs (`getpid()`) and Parent Process IDs (`getppid()`).
- Program execution and process address space replacement via `execvp()`.
- Parent-child synchronization and reaping zombie processes with `waitpid()`.
- Parsing termination statuses with `WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`.

## System Calls Used
- `fork()`: Creates a new child process duplicating the calling process.
- `execvp()`: Replaces current process memory image with executable binary.
- `waitpid()`: Suspends calling process until specified child changes state.
- `getpid()`: Retrieves process ID of caller.
- `getppid()`: Retrieves parent process ID of caller.
- `exit()`: Terminates calling process with status code.

## Architecture & Flow Diagram
```
[Parent Process (PID)]
        |
        +-- fork() --------> [Child 1 Process (PID)]
        |                           |
        |                           v
        |                     exit(42)
        |                           |
        +-- waitpid() <-------------+
        |
        +-- fork() --------> [Child 2 Process (PID)]
        |                           |
        |                           v
        |                     execvp("uname", "-a")
        |                           |
        +-- waitpid() <-------------+
        |
        +-- fork() x 3 ----> Process Tree (Children 3, 4, 5)
        |                           |
        +-- waitpid() x 3 <---------+
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o process_hierarchy
```

## How to Run
```bash
# Default run
./process_hierarchy

# With custom command arguments for Child 2
./process_hierarchy ls -la
```
