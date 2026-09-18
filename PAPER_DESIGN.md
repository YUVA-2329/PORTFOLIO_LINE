# PAPER DESIGN — Hand-copy this onto paper before submission

> The competition statement explicitly grades the paper design. This file is a clean reference/template; the final deliverable should be handwritten and photographed/scanned.

## 1. Problem understanding

Build a command-line line editor in C.
 The document is stored in memory.
  Commands operate on numbered lines rather than a free-moving 2D curs   or.

## 2. Data structure: Dynamic array of strings

```text
Document
+--------------------------+
| char **lines             |
| size_t count             |
| size_t capacity          |
+--------------------------+
             |
             v
     +---------------+
  1  | "First line"  |
     +---------------+
  2  | "Second line" |
     +---------------+
  3  | "Third line"  |
     +---------------+
```

### Why this choice?

- O(1) access by line number.
- O(n) insert/delete because lines shift.
- Dynamic growth means no fixed document limit.
- Easy to implement and explain within the competition time.

## 3. Command set

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

## 4. Core pseudocode

### Insert

```text
INSERT(line, text)
    if line < 1 OR line > count + 1
        report invalid line
        return
    make capacity for count + 1
    for i = count down to line
        lines[i] = lines[i - 1]
    lines[line - 1] = copy(text)
    count = count + 1
```

### Delete

```text
DELETE(line)
    if line < 1 OR line > count
        report invalid line
        return
    free(lines[line - 1])
    for i = line - 1 to count - 2
        lines[i] = lines[i + 1]
    count = count - 1
```

### Display

```text
DISPLAY()
    if count == 0
        print "Document is empty"
    else
        for i = 0 to count - 1
            print i + 1 and lines[i]
```

## 5. Bonus logic

```text
SEARCH(phrase)
    for every line
        if phrase occurs in line
            print line number

REPLACE(line, old, new)
    replace every occurrence of old on the selected line

REPLACEALL(old, new)
    replace old with new in every line

UNDO()
    restore the most recent saved document snapshot

STATS()
    count lines, words and characters
```

## 6. Edge cases

- Empty document.
- Insert into empty document at line 1.
- Insert at `count + 1` (append).
- Delete the only line.
- Delete first/middle/last line.
- Invalid line number.
- Missing file / permission error.
- Search phrase not found.
- Empty replacement phrase.
- Multiple occurrences of a phrase on one line.

## 7. Function plan

```text
document_init()
document_clear()
document_reserve()
document_copy()
document_insert()
document_delete()
document_save()
document_load()
replace_in_line()
replace_all()
count_words()
display_document()
print_help()
print_stats()
undo_push()
undo_pop()
command_loop_correct()
```
