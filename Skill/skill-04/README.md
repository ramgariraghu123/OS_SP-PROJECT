# Skill 04: Command Lexer & Tokenizer

## Concepts Covered
- Lexical analysis: converting raw character streams into typed token streams.
- Identifying and separating operators (`|`, `<`, `>`, `>>`, `&`) from words.
- Whitespace and delimiter handling.
- Syntax validation and identifying invalid syntax (leading/trailing pipes, dangling redirections).
- Generation of structured parse trees (`CommandNode` pipeline chain).
- Recursive memory cleanup of abstract syntax trees.

## System Calls & Standard Library APIs Used
- `malloc()`, `realloc()`, `calloc()`, `free()`: Dynamic memory allocation for tokens and nodes.
- `strdup()`, `strncpy()`, `isspace()`: String manipulation and classification.

## Architecture & Flow Diagram
```
Raw String: "cat file.txt | grep error > out.txt &"
                      |
                      v
             [Lexer / Tokenizer]
                      |
                      v
Tokens: [WORD: cat] [WORD: file.txt] [PIPE] [WORD: grep] [WORD: error] [REDIR_OUT: >] [WORD: out.txt] [BG: &]
                      |
                      v
               [Syntax Parser]
                      |
                      v
             [CommandNode Tree]
       Node 1: argv = ["cat", "file.txt"], pipe_next = Node 2
       Node 2: argv = ["grep", "error"], stdout = "out.txt", bg = true
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o command_tokenizer
```

## How to Run
```bash
./command_tokenizer
```
