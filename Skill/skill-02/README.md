# Skill 02: Interactive Shell Loop & Keyboard Input

## Concepts Covered
- Read-Eval-Print Loop (REPL) design and state control flow.
- Terminal character capture and canonical vs non-canonical mode.
- Low-level backspace processing (`\b \b`) and enter key handling (`\r`, `\n`).
- Detecting and processing EOF (Ctrl+D / end of file).
- Input buffer bounds management to prevent overflow.
- Built-in command processing and dispatch.

## System Calls & Library APIs Used
- `isatty()`: Tests whether a file descriptor refers to a terminal.
- `tcgetattr()`, `tcsetattr()`: Gets and sets terminal attributes for non-canonical raw character input.
- `getchar()`, `putchar()`: Character I/O primitives.
- `fgets()`: Safe line reading when operating in batch / piped mode.
- `getpid()`, `getppid()`: Process identification.

## Architecture & Flow Diagram
```
       +-----------------------+
       |   Initialize REPL     |
       +-----------+-----------+
                   |
     +------------>v---------------+
     |      Display Prompt         |
     |      "ossp-shell$ "         |
     |             |               |
     |             v               |
     |   Read Input Character      |
     |    (Backspace/Enter/EOF)    |
     |             |               |
     |   Enter pressed?            |
     |     /              \        |
     |   No               Yes      |
     |   /                  \      |
     | Append to buffer     Trim & Parse
     |   ^                   |
     +---+             Command == "exit" or EOF?
                       /        \
                     Yes         No
                     /             \
                  Terminate       Execute & Loop Back
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o interactive_loop
```

## How to Run
```bash
# Interactive mode
./interactive_loop

# Automated demonstration mode
./interactive_loop --demo

# Piped non-interactive mode
echo "status" | ./interactive_loop
```
