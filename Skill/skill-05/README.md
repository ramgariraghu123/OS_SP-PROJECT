# Skill 05: Quote Handling (Single and Double Quotes)

## Concepts Covered
- Lexical State Machine for quote parsing.
- Single Quote (`'...'`) semantics:
  - Exact literal string preservation.
  - Variable expansion suppression (`$VAR` is literal).
  - Escape sequences ignored.
- Double Quote (`"..."`) semantics:
  - Whitespace preservation inside words.
  - Environment variable expansion (`$VAR` expands to runtime value).
  - Selective backslash escapes (`\"`, `\$`, `\\`).
- Robust syntax error reporting on unclosed quotes.
- Clean heap allocation and string memory lifecycle.

## System Calls & Standard Library APIs Used
- `getenv()`: Retrieves environment variables during double quote expansion.
- `setenv()`: Sets environment variables for test harnesses.
- `malloc()`, `realloc()`, `free()`, `strdup()`: Dynamic string buffer and token array management.

## State Machine Diagram
```
                     +-------------------+
        +----------->|   STATE_NORMAL    |<-----------+
        |            +---+-----------+---+            |
        |                |           |                |
        |       '\''     |           |     '"'        |
        |                v           v                |
      '\''   +-----------------+   +-----------------+ '"'
        +----|  SINGLE_QUOTE   |   |  DOUBLE_QUOTE   |----+
             | Literal verbatim|   | Expand $VAR     |
             +-----------------+   +-----------------+
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o quote_parser
```

## How to Run
```bash
./quote_parser
```
