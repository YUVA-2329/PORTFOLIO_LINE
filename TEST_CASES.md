# TEST CASES

Run the editor and verify each expected result.

| # | Input | Expected |
|---|---|---|
| 1 | `display` on startup | `[Document is empty]` |
| 2 | `insert 1 Hello` | Line 1 contains `Hello` |
| 3 | `insert 2 World` | Line 2 contains `World` |
| 4 | `insert 2 Middle` | `Middle` becomes line 2; old line 2 shifts to line 3 |
| 5 | `delete 2` | Middle line removed; following lines shift up |
| 6 | `delete 99` | Error message; program continues |
| 7 | `insert 99 Bad` | Error message; program continues |
| 8 | `search "Hello"` | Reports line containing Hello |
| 9 | `search "missing"` | Reports no matching lines |
| 10 | `replace 1 "Hello" "Hi"` | Hello becomes Hi |
| 11 | `replaceall "o" "0"` | All occurrences across document are changed |
| 12 | `undo` | Previous mutation is restored |
| 13 | `stats` | Reports lines, words and characters |
| 14 | `save test.txt` | File is created with document lines |
| 15 | `load test.txt` | Document becomes file contents |
| 16 | `load missing.txt` | Error message; current document remains usable |
| 17 | `delete 1` when one line exists | Document becomes empty |
| 18 | `undo` after deleting only line | Deleted line returns |
| 19 | `help` | Complete command list appears |
| 20 | `quit` | Program exits cleanly |

## GCC warning test

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 main.c -o line_editor
```

The compile command should complete without warnings.
