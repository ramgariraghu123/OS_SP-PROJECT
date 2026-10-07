# Skill 08: Variable Expansion & Built-in Dispatch Table

## Concepts Covered
- Detecting variable substitution markers (`$VAR`, `$?`, `$$`).
- Environment variable lookup via `getenv()` and runtime state extraction.
- POSIX-compliant handling of undefined variables (expanding to empty string).
- In-process built-in command execution: why built-ins must run inside the shell parent process rather than a child.
- Function pointer dispatch table design (`BuiltinCommand` array).
- Maintaining shell state (last exit status, environment changes).

## System Calls & Standard Library APIs Used
- `getenv()`: Accesses environment variables for expansion.
- `setenv()`: Sets new or modified environment entries.
- `getpid()`: Resolves `$$` (current shell process ID).
- `strtok_r()`: Thread-safe and re-entrant string tokenization.
- `malloc()`, `realloc()`, `free()`: Safe dynamic buffer expansion.

## Architecture & Flow Diagram
```
Command string: "echo Host: $HOSTNAME PID: $$ Status: $?"
                         |
                         v
              [Variable Expansion Engine]
                         |
                         v
Result string:  "echo Host: ubuntu PID: 1350 Status: 0"
                         |
                         v
              [Tokenization (argv)]
                         |
                         v
              [Dispatch Table Lookup]
              Matches "echo" -> calls builtin_echo(argc, argv)
                         |
                         v
             Update last_exit_status = 0
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o var_dispatch_shell
```

## How to Run
```bash
./var_dispatch_shell
```
