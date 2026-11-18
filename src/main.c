#include "shell.h"

static bool is_word(const char *line, const char *word) {
    size_t n = strlen(word);
    return !strncmp(line, word, n) && (line[n] == '\0' || line[n] == ' ' || line[n] == '\t');
}

static void append_line(char ***lines, size_t *count, const char *line) {
    char **grown = realloc(*lines, (*count + 1) * sizeof(**lines));
    if (!grown) return;
    *lines = grown;
    (*lines)[(*count)++] = strdup(line);
}

static void free_lines(char **lines, size_t count) {
    for (size_t i = 0; i < count; ++i) free(lines[i]);
    free(lines);
}

static void run_if_block(char *first_line) {
    char *condition = first_line + 2;
    while (*condition == ' ' || *condition == '\t') ++condition;
    char **then_lines = NULL, **else_lines = NULL;
    size_t then_count = 0, else_count = 0;
    bool in_else = false, closed = false;
    char *line = NULL;
    while ((line = read_cmd("if> ")) != NULL) {
        if (is_word(line, "then")) { free(line); continue; }
        if (is_word(line, "else")) { in_else = true; free(line); continue; }
        if (!strcmp(line, "fi")) { closed = true; free(line); break; }
        if (in_else) append_line(&else_lines, &else_count, line);
        else append_line(&then_lines, &then_count, line);
        free(line);
    }
    if (!closed) fprintf(stderr, "syntax error: missing fi\n");
    else {
        int status = execute_line(condition);
        char **chosen = status == 0 ? then_lines : else_lines;
        size_t count = status == 0 ? then_count : else_count;
        for (size_t i = 0; i < count; ++i) execute_line(chosen[i]);
    }
    free_lines(then_lines, then_count);
    free_lines(else_lines, else_count);
}

int main(void) {
    char *line;
    while ((line = read_cmd(PROMPT)) != NULL) {
        reap_jobs();
        if (line[0] == '!') {
            char *expanded = history_command(line);
            free(line);
            line = expanded;
            if (!line || !*line) { free(line); continue; }
        }
        if (*line) remember_command(line);
        if (is_word(line, "if")) run_if_block(line);
        else if (*line) execute_line(line);
        free(line);
    }
    puts("\nShell exited.");
    return 0;
}
