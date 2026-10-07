# Skill 10: Built-in Commands & Child Environment Passing

## Concepts Covered
- Implementing essential built-in commands: `pwd`, `export`, and `exit`.
- Validating variable naming syntax (POSIX identifier rules).
- Modifying current process environment table with `setenv()`.
- Verifying environment persistence across `fork()` into child processes.
- Verifying environment inheritance across `execvp()` execution boundaries.
- Exit code handling and clean shutdown procedures.

## System Calls & Standard Library APIs Used
- `setenv()`: Inserts or updates variable in process environment.
- `getenv()`: Reads process environment variable.
- `getcwd()`: Gets current working directory pathname.
- `fork()`: Creates child process inherits copy of environment.
- `execvp()`: Executes program with inherited environment table.
- `waitpid()`: Synchronizes parent with child completion.

## Architecture & Flow Diagram
```
Shell Process Environment:
+------------------------------------+
| PWD=/home/raghu                    |
| OSSP_LAB_TOKEN=X9988_TOKEN         |
+------------------------------------+
                  |
             fork()
            /      \
           v        v
      [Parent]    [Child Process]
                  Inherits identical copy of environment:
                  getenv("OSSP_LAB_TOKEN") -> "X9988_TOKEN"
                        |
                     execvp()
                        |
                  [New Binary]
                  Still has access to inherited environment!
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o builtin_env_manager
```

## How to Run
```bash
./builtin_env_manager
```
