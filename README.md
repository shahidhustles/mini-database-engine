# Mini DB Engine in C++

This project is a small course-style database engine written in C++.

It is not trying to be MySQL or SQLite. The goal is to demonstrate core data-structure ideas in one working system:

- `Red-Black Tree` for fast exact lookup in memory
- `Skip List` for ordered traversal in memory
- `B+ Tree` for disk-backed sorted storage
- `Huffman Coding` for value compression

The program runs as a simple CLI REPL and supports:

- `INSERT`
- `GET`
- `RANGE`
- `FLUSH`
- `EXIT`

## What the project does

The database stores:

- `int` keys
- `string` values

Behavior:

- `INSERT` writes to the memory layer first
- `GET` checks memory first, then disk
- `RANGE` merges memory and disk results
- `FLUSH` writes memory records to disk
- `EXIT` also flushes before closing
- inserting the same key again overwrites the old value

## Architecture

The layers are intentionally separated:

```text
CLI / REPL
   |
   v
DatabaseInterface
   |         \
   v          v
MemoryLayer   StorageLayer
   |              |
   v              v
RBTree + SkipList B+ Tree

HuffmanCodec is used by DatabaseInterface before write and after read.
```

### Layer responsibilities

- `main.cpp`
  - reads user input
  - parses commands
  - calls `DatabaseInterface`

- `db_interface.*`
  - coordinates everything
  - decides whether to use memory or storage
  - compresses before storing
  - decompresses before returning values

- `memory_layer.*`
  - writes every in-memory record to both:
    - Red-Black Tree
    - Skip List
  - Red-Black Tree is used for point lookup
  - Skip List is used for ordered traversal and range snapshots

- `storage_layer.*`
  - manages the disk files
  - stores compressed values in `values.db`
  - stores the B+ Tree index in `index.db`

- `huffman.*`
  - pure compression utility
  - has no knowledge of the database layers

## Project structure

```text
.
├── CMakeLists.txt
├── include/
│   ├── bplus_tree.hpp
│   ├── command_parser.hpp
│   ├── compressed_value.hpp
│   ├── db_interface.hpp
│   ├── huffman.hpp
│   ├── memory_layer.hpp
│   ├── red_black_tree.hpp
│   ├── skip_list.hpp
│   └── storage_layer.hpp
├── src/
│   ├── bplus_tree.cpp
│   ├── command_parser.cpp
│   ├── db_interface.cpp
│   ├── huffman.cpp
│   ├── main.cpp
│   ├── memory_layer.cpp
│   ├── red_black_tree.cpp
│   ├── skip_list.cpp
│   └── storage_layer.cpp
└── tests/
    └── test_main.cpp
```

## Build

### Requirements

- C++17 compiler
- CMake

### Configure and build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

This produces:

- `build/mini_db_engine`
- `build/mini_db_engine_tests`

## Run

Start the CLI:

```bash
./build/mini_db_engine
```

Example session:

```text
db> INSERT 5 "hello world"
OK
db> GET 5
hello world
db> RANGE 1 10
5 => hello world
db> FLUSH
OK
db> EXIT
BYE
```

## Supported commands

### `INSERT <key> "<value>"`

Example:

```text
INSERT 10 "apple"
```

Stores key `10` with value `"apple"`.

If key `10` already exists, the value is overwritten.

### `GET <key>`

Example:

```text
GET 10
```

Returns the value for key `10`, or `NOT FOUND`.

### `RANGE <start> <end>`

Example:

```text
RANGE 3 7
```

Returns all key-value pairs in the inclusive range `[3, 7]`.

### `FLUSH`

Writes all current in-memory records to disk.

### `EXIT`

Flushes memory to disk and exits.

## Tests

Build first, then run:

```bash
./build/mini_db_engine_tests
```

The test binary covers:

- Huffman compression/decompression
- Red-Black Tree insert/find/overwrite
- Skip List order/range behavior
- Memory layer snapshots
- B+ Tree splits and range scans
- Storage persistence across reopen
- DatabaseInterface integration
- Command parsing

## Disk files

The storage layer uses two binary files:

- `index.db`
  - stores the B+ Tree nodes
- `values.db`
  - stores compressed values

The values file is append-only in this MVP.

That means overwriting a key writes a new value record and updates the B+ Tree pointer. Old value bytes may remain in the file unused. That is acceptable for this course version.

## How to study this code

If you have not studied all the data structures yet, read the code in this order:

1. `src/main.cpp`
2. `src/command_parser.cpp`
3. `src/db_interface.cpp`
4. `src/memory_layer.cpp`
5. `src/huffman.cpp`
6. `src/red_black_tree.cpp`
7. `src/skip_list.cpp`
8. `src/storage_layer.cpp`
9. `src/bplus_tree.cpp`

Why this order:

- first understand the user flow
- then understand the orchestration
- then study one data structure at a time
- leave the B+ Tree for later because it is the most involved part

## Important limitations

This is intentionally an MVP:

- no delete
- no transaction support
- no concurrency
- no crash recovery
- no write-ahead log
- no SQL parser
- no background flushing

## Notes for a course demo

If you need to explain the project in class, a clean summary is:

> The database interface coordinates four data-structure components. New writes go to memory first, exact reads use a Red-Black Tree, ordered memory traversal uses a Skip List, durable storage uses a B+ Tree, and values are compressed with Huffman coding before storage.

