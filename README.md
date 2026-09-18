# Simple Line Editor in C

A terminal-only line editor built for the **Portfolio Building — Studio Course 3rd Semester Coding Competition**.

## Team

- Team member 1: ____________________
- Team member 2: ____________________
- Team member 3: ____________________

## Implemented features

### Core
- [x] Insert a line
- [x] Delete a line
- [x] Display the document
- [x] Save / load a `.txt` file

### Bonus
- [x] Search
- [x] Find & replace
- [x] Undo last action
- [x] Line count / word count / character count

This implements all listed core and bonus features, rather than only the minimum 2–3 core features.

## Data structure choice

The editor stores lines in a **dynamic array of strings (`char **`)**.

Why:
- Simple to understand and justify in a short coding competition.
- Fast random access by line number: `O(1)` lookup.
- Insert/delete require shifting lines: `O(n)` in the worst case.
- Dynamic capacity growth avoids a fixed maximum number of lines.
- Each line is independently allocated, so line lengths are not fixed.

A linked list would make insertion/deletion cheaper once a node is found, but direct access to a numbered line would be slower. For a small terminal document, the dynamic array gives a clean balance of simplicity and performance.

## Undo design

Undo stores deep copies (snapshots) of the document before each mutating operation. A maximum of 50 snapshots is retained. This makes undo reliable and easy to explain during the viva/demo, at the cost of additional memory.

## Commands

See [`HELP.md`](HELP.md) for the full command reference and examples.

Quick list:

```text
insert <line> <text>
delete <line>
display
save <file.txt>
load <file.txt>
search "phrase"
replace <line> "old" "new"
replaceall "old" "new"
undo
stats
help
quit
```

## Compile with GCC

Linux/macOS/WSL:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 main.c -o line_editor
```

Or, if using the supplied Makefile:

```bash
make
```

## Run

Start empty:

```bash
./line_editor
```

Start by loading an existing text file:

```bash
./line_editor notes.txt
```

The startup-file behavior directly supports reading a `.txt` document into memory when the editor starts.

## Demo sequence

```text
insert 1 Hello from our line editor
insert 2 This is the second line
insert 2 A new line inserted in the middle
display
search "line"
replace 1 "line editor" "C editor"
replaceall "the" "THE"
stats
save demo.txt
delete 2
display
undo
display
quit
```

## Repository structure

```text
line-editor/
├── main.c
├── HELP.md
├── README.md
├── PAPER_DESIGN.md
├── TEST_CASES.md
├── Makefile
└── .gitignore
```

## Submission checklist

- [ ] Add real team member names above.
- [ ] Hand-write the paper design before coding and submit a photo/scan as required.
- [ ] Build with GCC without warnings.
- [ ] Run the test cases in `TEST_CASES.md`.
- [ ] Commit the project and push it to GitHub.
- [ ] Keep meaningful commits showing team participation.
