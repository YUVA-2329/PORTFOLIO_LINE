#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>

#define INITIAL_CAPACITY 8
#define MAX_UNDO 50
#define INPUT_SIZE 4096

/*
 * Simple line editor implemented with a dynamic array of strings.
 * Lines are numbered from 1 for the user; internally the array is 0-based.
 */
typedef struct {
    char **lines;
    size_t count;
    size_t capacity;
} Document;

typedef struct {
    Document snapshots[MAX_UNDO];
    size_t count;
} UndoStack;

static void die(const char *message) {
    fprintf(stderr, "Fatal error: %s\n", message);
    exit(EXIT_FAILURE);
}

static char *duplicate_string(const char *src) {
    size_t len = strlen(src);
    char *copy = malloc(len + 1);
    if (!copy) die("out of memory");
    memcpy(copy, src, len + 1);
    return copy;
}

static void document_init(Document *doc) {
    doc->lines = NULL;
    doc->count = 0;
    doc->capacity = 0;
}

static void document_clear(Document *doc) {
    for (size_t i = 0; i < doc->count; ++i) {
        free(doc->lines[i]);
    }
    free(doc->lines);
    document_init(doc);
}

static void document_reserve(Document *doc, size_t needed) {
    if (needed <= doc->capacity) return;
    size_t new_capacity = doc->capacity ? doc->capacity : INITIAL_CAPACITY;
    while (new_capacity < needed) new_capacity *= 2;

    char **new_lines = realloc(doc->lines, new_capacity * sizeof(*new_lines));
    if (!new_lines) die("out of memory");
    doc->lines = new_lines;
    doc->capacity = new_capacity;
}

static void document_copy(Document *dest, const Document *src) {
    document_init(dest);
    document_reserve(dest, src->count);
    for (size_t i = 0; i < src->count; ++i) {
        dest->lines[i] = duplicate_string(src->lines[i]);
    }
    dest->count = src->count;
}

static void undo_init(UndoStack *stack) {
    stack->count = 0;
    for (size_t i = 0; i < MAX_UNDO; ++i) document_init(&stack->snapshots[i]);
}

static void undo_clear(UndoStack *stack) {
    for (size_t i = 0; i < stack->count; ++i) document_clear(&stack->snapshots[i]);
    stack->count = 0;
}

static void undo_push(UndoStack *stack, const Document *doc) {
    if (stack->count == MAX_UNDO) {
        document_clear(&stack->snapshots[0]);
        for (size_t i = 1; i < MAX_UNDO; ++i) {
            stack->snapshots[i - 1] = stack->snapshots[i];
        }
        document_init(&stack->snapshots[MAX_UNDO - 1]);
        stack->count--;
    }
    document_copy(&stack->snapshots[stack->count], doc);
    stack->count++;
}

static int undo_pop(UndoStack *stack, Document *current) {
    if (stack->count == 0) return 0;
    Document previous = stack->snapshots[stack->count - 1];
    document_init(&stack->snapshots[stack->count - 1]);
    stack->count--;
    document_clear(current);
    *current = previous;
    return 1;
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

static char *trim_whitespace(char *s) {
    while (isspace((unsigned char)*s)) ++s;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

static int parse_positive_number(const char *text, size_t *value) {
    if (!text || !*text) return 0;
    char *end;
    errno = 0;
    unsigned long long n = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || n == 0) return 0;
    if (n > (unsigned long long)SIZE_MAX) return 0;
    *value = (size_t)n;
    return 1;
}

/* Extract the next whitespace-delimited token. Modifies the input string. */
static char *next_token(char **cursor) {
    char *p = *cursor;
    while (*p && isspace((unsigned char)*p)) ++p;
    if (!*p) {
        *cursor = p;
        return NULL;
    }
    char *start = p;
    while (*p && !isspace((unsigned char)*p)) ++p;
    if (*p) *p++ = '\0';
    *cursor = p;
    return start;
}

/* Parse a quoted or unquoted argument and advance cursor. Quotes are removed. */
static char *next_argument(char **cursor) {
    char *p = *cursor;
    while (*p && isspace((unsigned char)*p)) ++p;
    if (!*p) {
        *cursor = p;
        return NULL;
    }

    if (*p == '"') {
        ++p;
        char *start = p;
        char *out = start;
        while (*p) {
            if (*p == '"') {
                *out = '\0';
                ++p;
                *cursor = p;
                return start;
            }
            if (*p == '\\' && p[1] == '"') {
                *out++ = '"';
                p += 2;
            } else {
                *out++ = *p++;
            }
        }
        *out = '\0';
        *cursor = p;
        return start;
    }

    char *start = p;
    while (*p && !isspace((unsigned char)*p)) ++p;
    if (*p) *p++ = '\0';
    *cursor = p;
    return start;
}

static int document_insert(Document *doc, size_t line_number, const char *text) {
    if (line_number < 1 || line_number > doc->count + 1) return 0;
    document_reserve(doc, doc->count + 1);
    size_t index = line_number - 1;
    for (size_t i = doc->count; i > index; --i) {
        doc->lines[i] = doc->lines[i - 1];
    }
    doc->lines[index] = duplicate_string(text ? text : "");
    doc->count++;
    return 1;
}

static int document_delete(Document *doc, size_t line_number) {
    if (line_number < 1 || line_number > doc->count) return 0;
    size_t index = line_number - 1;
    free(doc->lines[index]);
    for (size_t i = index; i + 1 < doc->count; ++i) {
        doc->lines[i] = doc->lines[i + 1];
    }
    doc->count--;
    return 1;
}

static size_t replace_in_line(char **line_ptr, const char *old_text, const char *new_text) {
    if (!old_text || !*old_text) return 0;
    char *line = *line_ptr;
    size_t old_len = strlen(old_text);
    size_t new_len = strlen(new_text);
    size_t occurrences = 0;

    for (char *p = line; (p = strstr(p, old_text)) != NULL; p += old_len) occurrences++;
    if (occurrences == 0) return 0;

    size_t line_len = strlen(line);
    size_t new_len_total;
    if (new_len >= old_len) {
        new_len_total = line_len + occurrences * (new_len - old_len);
    } else {
        new_len_total = line_len - occurrences * (old_len - new_len);
    }
    char *result = malloc(new_len_total + 1);
    if (!result) die("out of memory");

    char *src = line;
    char *dst = result;
    while (1) {
        char *match = strstr(src, old_text);
        if (!match) {
            strcpy(dst, src);
            break;
        }
        size_t prefix = (size_t)(match - src);
        memcpy(dst, src, prefix);
        dst += prefix;
        memcpy(dst, new_text, new_len);
        dst += new_len;
        src = match + old_len;
    }

    free(line);
    *line_ptr = result;
    return occurrences;
}

static size_t replace_all(Document *doc, const char *old_text, const char *new_text, size_t *changed_lines) {
    size_t total = 0;
    if (changed_lines) *changed_lines = 0;
    for (size_t i = 0; i < doc->count; ++i) {
        size_t replaced = replace_in_line(&doc->lines[i], old_text, new_text);
        total += replaced;
        if (replaced && changed_lines) (*changed_lines)++;
    }
    return total;
}

static int document_save(const Document *doc, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) return 0;
    for (size_t i = 0; i < doc->count; ++i) {
        if (fprintf(file, "%s\n", doc->lines[i]) < 0) {
            fclose(file);
            return 0;
        }
    }
    if (fclose(file) != 0) return 0;
    return 1;
}

static int document_load(Document *doc, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;

    Document loaded;
    document_init(&loaded);
    char buffer[INPUT_SIZE];

    while (fgets(buffer, sizeof(buffer), file)) {
        trim_newline(buffer);
        if (!document_insert(&loaded, loaded.count + 1, buffer)) {
            document_clear(&loaded);
            fclose(file);
            return 0;
        }
    }

    if (ferror(file)) {
        document_clear(&loaded);
        fclose(file);
        return 0;
    }
    fclose(file);

    document_clear(doc);
    *doc = loaded;
    return 1;
}

static size_t count_words(const char *text) {
    size_t count = 0;
    int in_word = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (isspace(*p)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            count++;
        }
    }
    return count;
}

static void display_document(const Document *doc) {
    if (doc->count == 0) {
        printf("[Document is empty]\n");
        return;
    }
    for (size_t i = 0; i < doc->count; ++i) {
        printf("%4zu | %s\n", i + 1, doc->lines[i]);
    }
}

static void print_help(void) {
    puts("\nLINE EDITOR COMMANDS");
    puts("--------------------");
    puts("insert <line> <text>             Insert text at a line number.");
    puts("delete <line>                    Delete a line.");
    puts("display                           Display all lines with numbers.");
    puts("save <file.txt>                   Save the document to a .txt file.");
    puts("load <file.txt>                   Load a .txt file into the editor.");
    puts("search \"phrase\"                 Show line numbers containing a phrase.");
    puts("replace <line> \"old\" \"new\"    Replace phrase(s) on one line.");
    puts("replaceall \"old\" \"new\"        Replace phrase(s) throughout document.");
    puts("undo                              Undo the most recent change.");
    puts("stats                             Show line, word and character counts.");
    puts("help                              Show this help.");
    puts("quit                              Exit the editor.");
    puts("\nNotes: line numbers start at 1. For phrases with spaces, use double quotes.");
}

static void print_stats(const Document *doc) {
    size_t words = 0;
    size_t chars = 0;
    for (size_t i = 0; i < doc->count; ++i) {
        words += count_words(doc->lines[i]);
        chars += strlen(doc->lines[i]);
    }
    printf("Lines: %zu\nWords: %zu\nCharacters: %zu\n", doc->count, words, chars);
}

/* Corrected replace command with undo snapshot taken before mutation. */
static void run_replace_command(Document *doc, UndoStack *undo, char *cursor) {
    char *line_text = next_token(&cursor);
    char *old_text = next_argument(&cursor);
    char *new_text = next_argument(&cursor);
    size_t line_number;
    if (!line_text || !parse_positive_number(line_text, &line_number) || !old_text || !new_text || !*old_text) {
        puts("Usage: replace <line> \"old\" \"new\"");
        return;
    }
    if (line_number > doc->count) {
        printf("Invalid line number. Replace range: 1-%zu.\n", doc->count);
        return;
    }

    Document before;
    document_copy(&before, doc);
    size_t replaced = replace_in_line(&doc->lines[line_number - 1], old_text, new_text);
    if (!replaced) {
        document_clear(&before);
        printf("'%s' was not found on line %zu.\n", old_text, line_number);
        return;
    }
    undo_push(undo, &before);
    document_clear(&before);
    printf("Replaced %zu occurrence(s) on line %zu.\n", replaced, line_number);
}

/* Main command loop, separated so replace can capture a pre-mutation snapshot. */
static void command_loop_correct(Document *doc, UndoStack *undo) {
    char input[INPUT_SIZE];
    puts("\nSimple Line Editor in C");
    puts("Type 'help' for commands. Type 'quit' to exit.");

    for (;;) {
        printf("\neditor> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        trim_newline(input);
        char *command_line = trim_whitespace(input);
        if (!*command_line) continue;

        char *cursor = command_line;
        char *command = next_token(&cursor);
        if (!command) continue;

        if (strcmp(command, "insert") == 0) {
            char *line_text = next_token(&cursor);
            char *text = trim_whitespace(cursor);
            size_t line_number;
            if (!line_text || !parse_positive_number(line_text, &line_number) || !*text) {
                puts("Usage: insert <line> <text>");
                continue;
            }
            if (line_number > doc->count + 1) {
                printf("Invalid line number. Insert range: 1-%zu.\n", doc->count + 1);
                continue;
            }
            undo_push(undo, doc);
            document_insert(doc, line_number, text);
            printf("Inserted line %zu.\n", line_number);
        } else if (strcmp(command, "delete") == 0) {
            char *line_text = next_token(&cursor);
            size_t line_number;
            if (!line_text || !parse_positive_number(line_text, &line_number)) {
                puts("Usage: delete <line>");
                continue;
            }
            if (line_number > doc->count) {
                printf("Invalid line number. Delete range: 1-%zu.\n", doc->count);
                continue;
            }
            undo_push(undo, doc);
            document_delete(doc, line_number);
            printf("Deleted line %zu.\n", line_number);
        } else if (strcmp(command, "display") == 0 || strcmp(command, "list") == 0) {
            display_document(doc);
        } else if (strcmp(command, "save") == 0) {
            char *filename = next_argument(&cursor);
            if (!filename || !*filename) {
                puts("Usage: save <file.txt>");
                continue;
            }
            if (document_save(doc, filename)) printf("Saved %zu line(s) to '%s'.\n", doc->count, filename);
            else printf("Could not save '%s'. Check the path and permissions.\n", filename);
        } else if (strcmp(command, "load") == 0) {
            char *filename = next_argument(&cursor);
            if (!filename || !*filename) {
                puts("Usage: load <file.txt>");
                continue;
            }
            Document loaded;
            document_init(&loaded);
            if (!document_load(&loaded, filename)) {
                document_clear(&loaded);
                printf("Could not load '%s'.\n", filename);
                continue;
            }
            undo_push(undo, doc);
            document_clear(doc);
            *doc = loaded;
            printf("Loaded %zu line(s) from '%s'.\n", doc->count, filename);
        } else if (strcmp(command, "search") == 0) {
            char *phrase = next_argument(&cursor);
            if (!phrase || !*phrase) {
                puts("Usage: search \"phrase\"");
                continue;
            }
            size_t matches = 0;
            for (size_t i = 0; i < doc->count; ++i) {
                if (strstr(doc->lines[i], phrase)) {
                    printf("Line %zu: %s\n", i + 1, doc->lines[i]);
                    matches++;
                }
            }
            if (!matches) printf("No lines contain '%s'.\n", phrase);
            else printf("Found %zu matching line(s).\n", matches);
        } else if (strcmp(command, "replace") == 0) {
            run_replace_command(doc, undo, cursor);
        } else if (strcmp(command, "replaceall") == 0) {
            char *old_text = next_argument(&cursor);
            char *new_text = next_argument(&cursor);
            if (!old_text || !new_text || !*old_text) {
                puts("Usage: replaceall \"old\" \"new\"");
                continue;
            }
            Document before;
            document_copy(&before, doc);
            size_t changed_lines = 0;
            size_t replaced = replace_all(doc, old_text, new_text, &changed_lines);
            if (!replaced) {
                document_clear(&before);
                printf("'%s' was not found in the document.\n", old_text);
            } else {
                undo_push(undo, &before);
                document_clear(&before);
                printf("Replaced %zu occurrence(s) across %zu line(s).\n", replaced, changed_lines);
            }
        } else if (strcmp(command, "undo") == 0) {
            if (undo_pop(undo, doc)) puts("Last change undone.");
            else puts("Nothing to undo.");
        } else if (strcmp(command, "stats") == 0) {
            print_stats(doc);
        } else if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;
        } else {
            printf("Unknown command '%s'. Type 'help' for the command list.\n", command);
        }
    }
}

int main(int argc, char *argv[]) {
    Document doc;
    UndoStack undo;
    document_init(&doc);
    undo_init(&undo);

    /* Optional startup file: ./line_editor document.txt */
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [file.txt]\n", argv[0]);
        undo_clear(&undo);
        document_clear(&doc);
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        if (document_load(&doc, argv[1])) {
            printf("Loaded %zu line(s) from startup file '%s'.\n", doc.count, argv[1]);
        } else {
            fprintf(stderr, "Warning: could not load '%s'; starting with an empty document.\n", argv[1]);
        }
    }

    command_loop_correct(&doc, &undo);
    undo_clear(&undo);
    document_clear(&doc);
    return EXIT_SUCCESS;
}
