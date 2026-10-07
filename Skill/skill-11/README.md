# Skill 11: History Buffer & Pipeline Data Structures

## Concepts Covered
- Fixed-capacity circular ring buffer architecture.
- FIFO eviction policy for oldest entries when history limit is reached.
- Command indexing and fast recall (`!id`, `!-1`).
- Modeling Unix shell pipelines as linked stage nodes.
- Preserving ordered execution flows: Stage 1 output -> Stage 2 input.
- Memory lifecycle validation: safe string duplication and node deallocation.

## Data Structures Used
```c
typedef struct {
    HistoryEntry entries[MAX_HISTORY_CAPACITY];
    int start;
    int count;
    int next_id;
} HistoryRingBuffer;

typedef struct PipelineStage {
    int stage_id;
    char **argv;
    int argc;
    struct PipelineStage *next;
} PipelineStage;
```

## Architecture & Flow Diagram
```
Command Line: "ps -ef | grep systemd | sort"
                       |
                       v
              [Pipeline Parser]
                       |
                       v
       +-------------------------------+
       | Stage 1: ["ps", "-ef"]        | ===(pipe)===>
       +-------------------------------+
       | Stage 2: ["grep", "systemd"]  | ===(pipe)===>
       +-------------------------------+
       | Stage 3: ["sort"]             | ===(stdout)==> Terminal
       +-------------------------------+
```

## How to Build
```bash
# Using Makefile
make

# Or directly with gcc
gcc *.c -o history_pipeline_struct
```

## How to Run
```bash
./history_pipeline_struct
```
