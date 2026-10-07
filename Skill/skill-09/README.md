# Skill 09: Directory Navigation & State Management

## Concepts Covered
- In-process directory changes via `chdir()`.
- Tracking current working directory using `getcwd()`.
- Updating environment variables `PWD` and `OLDPWD` via `setenv()`.
- Handling relative paths (`..`, `.`), absolute paths (`/tmp`), and home shortcuts (`~` or empty argument).
- Implementing toggling navigation (`cd -`).
- Graceful error detection with `perror()` / `strerror()` (e.g. `ENOENT`, `ENOTDIR`, `EACCES`).

## System Calls Used
- `chdir()`: Changes process current working directory.
- `getcwd()`: Gets pathname of current working directory.
- `setenv()`: Updates `PWD` and `OLDPWD` environment variables.
- `getenv()`: Retrieves `HOME` and `OLDPWD` environment variables.

## Architecture & Flow Diagram
```
Command: "cd /tmp"
       |
       v
Check target path
       |
       +---> chdir(target) != 0 ? ---> Report error (ENOENT/ENOTDIR)
       |
       +---> Success:
                1. OLDPWD = current_pwd
                2. getcwd(current_pwd)
                3. setenv("PWD", current_pwd)
                4. setenv("OLDPWD", OLDPWD)
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o cd_navigator
```

## How to Run
```bash
./cd_navigator
```
