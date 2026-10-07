# Skill 03: Dynamic Buffer Management & History Navigation

## Concepts Covered
- Dynamic memory management (`malloc`, `realloc`, `free`) with geometric growth.
- Buffer overflow prevention through size tracking and dynamic resizing.
- Doubly linked list design for bidirectional command history.
- Handling ANSI escape sequences (`\033[A` for Up, `\033[B` for Down).
- In-memory command recall and real-time buffer updating.
- Complete resource deallocation to ensure zero memory leaks.

## System Calls & Standard Library APIs Used
- `malloc()`, `realloc()`, `free()`: Dynamic heap memory management.
- `tcgetattr()`, `tcsetattr()`: Raw terminal mode configuration.
- `getchar()`, `putchar()`: Low-level character I/O.
- `isatty()`: Checking for terminal device vs automated script.

## Architecture & Flow Diagram
```
       +---------------------------------------------+
       |   DynBuffer: capacity * 2 on overflow       |
       +---------------------------------------------+
                            |
   +------------------------v------------------------+
   |          History Linked List (Doubly)           |
   |                                                 |
   |   [Node 1] <===> [Node 2] <===> [Node 3 (tail)] |
   +-------------------------------------------------+
          ^                                 |
          |                                 | Up Arrow (\033[A)
          +------ Down Arrow (\033[B) <-----+
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o dynamic_buffer_history
```

## How to Run
```bash
# Automated demonstration mode
./dynamic_buffer_history --demo

# Interactive mode
./dynamic_buffer_history
```
