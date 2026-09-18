# HELP — Simple Line Editor in C

Line numbers start at **1**. The editor operates on complete lines from the terminal.

## Commands

### 1. Insert a line
```text
insert <line> <text>
```
Adds a new line at the given position and shifts existing lines down.

Example:
```text
insert 1 Hello world
insert 2 This is line two
insert 2 Inserted between them
```

### 2. Delete a line
```text
delete <line>
```
Removes the specified line and shifts following lines up.

Example:
```text
delete 2
```

### 3. Display the document
```text
display
```
Prints every current line with its line number.

Example output:
```text
   1 | Hello world
   2 | This is line two
```

### 4. Save a file
```text
save <file.txt>
```
Writes the in-memory document to a text file.

Example:
```text
save notes.txt
```

### 5. Load a file
```text
load <file.txt>
```
Replaces the current document with the contents of the text file.

Example:
```text
load notes.txt
```

A startup file can also be supplied when launching:
```bash
./line_editor notes.txt
```

### 6. Search
```text
search "phrase"
```
Reports every line containing the given word or phrase.

Example:
```text
search "data structure"
```

### 7. Find & replace on one line
```text
replace <line> "old" "new"
```
Replaces every occurrence of `old` on the specified line.

Example:
```text
replace 2 "old name" "new name"
```

### 8. Find & replace across the document
```text
replaceall "old" "new"
```
Replaces every occurrence across every line.

Example:
```text
replaceall "C language" "C programming"
```

### 9. Undo
```text
undo
```
Reverses the most recent mutating operation. Up to 50 previous document states are kept.

Example:
```text
insert 3 Temporary line
undo
```

### 10. Statistics
```text
stats
```
Reports line count, word count and character count.

Example:
```text
stats
```

### 11. Help
```text
help
```
Shows the command list.

### 12. Quit
```text
quit
```
Exits the editor. `exit` is also accepted.

## Error handling
- Invalid line numbers are rejected without crashing.
- Insertion accepts positions `1` through `line_count + 1`.
- Deletion and single-line replacement require an existing line.
- Empty documents can be displayed safely.
- Missing files and file permission errors are reported gracefully.
- Empty search/replace phrases are rejected.
- Phrases containing spaces should be enclosed in double quotes.
