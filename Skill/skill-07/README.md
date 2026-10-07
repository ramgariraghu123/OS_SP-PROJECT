# Skill 07: Process Synchronization & PATH Resolution

## Concepts Covered
- PATH environment variable traversal and colon-delimited token parsing.
- Executable verification using `access(candidate, X_OK)`.
- Distinguishing relative/absolute paths (`./` or `/`) from PATH lookups.
- Direct binary invocation via `execv()`.
- Process synchronization with `waitpid()`.
- Detailed exit analysis using macros: `WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`, `WTERMSIG`, `WIFSTOPPED`.

## System Calls Used
- `access()`: Tests file accessibility and execution permission.
- `fork()`: Creates child process.
- `execv()`: Executes binary directly at absolute resolved path.
- `waitpid()`: Awaits specific child termination and extracts exit status flags.
- `getenv()`: Retrieves `PATH` variable from environment.

## Architecture & Flow Diagram
```
Command name: "whoami"
          |
          v
   [PATH Traverser]
   Extracts: /usr/local/bin, /usr/bin, /bin ...
          |
          v
   Test: access("/usr/bin/whoami", X_OK) == 0 ?
          |
          +--> Found: "/usr/bin/whoami"
          |
        fork()
       /      \
      v        v
   [Parent]   [Child]
      |          |
      |       execv("/usr/bin/whoami", argv)
      |          |
   waitpid() <---+
      |
   Status: WIFEXITED == true, WEXITSTATUS == 0
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o path_sync_exec
```

## How to Run
```bash
# Automated tests
./path_sync_exec

# Run specific command
./path_sync_exec ls -l
```
