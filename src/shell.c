#include "shell.h"
#include <dirent.h>
#include <ctype.h>

static char **completion_items;
static size_t completion_count;
static size_t completion_pos;

static char *completion_generator(const char *text, int state) {
    if (!state) completion_pos = 0;
    size_t n = strlen(text);
    while (completion_pos < completion_count) {
        const char *item = completion_items[completion_pos++];
        if (!strncmp(item, text, n)) return strdup(item);
    }
    return NULL;
}

static char **complete_input(const char *text, int start, int end) {
    (void)end;
    for (size_t i = 0; i < completion_count; ++i) free(completion_items[i]);
    free(completion_items);
    completion_items = NULL;
    completion_count = 0;
    if (start != 0) return rl_completion_matches(text, rl_filename_completion_function);

    char *path = strdup(getenv("PATH") ? getenv("PATH") : "");
    if (!path) return NULL;
    char *save = NULL;
    for (char *dir = strtok_r(path, ":", &save); dir; dir = strtok_r(NULL, ":", &save)) {
        DIR *dp = opendir(*dir ? dir : ".");
        if (!dp) continue;
        struct dirent *entry;
        while ((entry = readdir(dp))) {
            size_t len = strlen(dir) + strlen(entry->d_name) + 2;
            char *full = malloc(len);
            if (!full) continue;
            snprintf(full, len, "%s/%s", *dir ? dir : ".", entry->d_name);
            if (access(full, X_OK) == 0 && entry->d_name[0] != '.') {
                char **grown = realloc(completion_items, (completion_count + 1) * sizeof(*grown));
                if (grown) { completion_items = grown; completion_items[completion_count++] = strdup(entry->d_name); }
            }
            free(full);
        }
        closedir(dp);
    }
    const char *builtins[] = {"cd", "exit", "help", "history", "jobs", "set", NULL};
    for (size_t i = 0; builtins[i]; ++i) {
        char **grown = realloc(completion_items, (completion_count + 1) * sizeof(*grown));
        if (grown) { completion_items = grown; completion_items[completion_count++] = strdup(builtins[i]); }
    }
    free(path);
    return rl_completion_matches(text, completion_generator);
}

char *read_cmd(const char *prompt) {
    rl_attempted_completion_function = complete_input;
    return readline(prompt);
}

char **tokenize(const char *line) {
    size_t cap = 16, count = 0;
    char **tokens = calloc(cap, sizeof(*tokens));
    if (!tokens) return NULL;
    char *copy = strdup(line);
    if (!copy) { free(tokens); return NULL; }
    char *save = NULL;
    for (char *word = strtok_r(copy, " \t", &save); word; word = strtok_r(NULL, " \t", &save)) {
        if (count + 1 >= cap) {
            cap *= 2;
            char **grown = realloc(tokens, cap * sizeof(*tokens));
            if (!grown) { free(copy); free_tokens(tokens); return NULL; }
            tokens = grown;
        }
        tokens[count++] = strdup(word);
    }
    free(copy);
    return tokens;
}

void free_tokens(char **tokens) {
    if (!tokens) return;
    for (size_t i = 0; tokens[i]; ++i) free(tokens[i]);
    free(tokens);
}

bool is_builtin(const char *name) {
    return name && (!strcmp(name, "exit") || !strcmp(name, "cd") ||
                    !strcmp(name, "help") || !strcmp(name, "jobs") ||
                    !strcmp(name, "history"));
}

int run_builtin(char **argv, bool parent_context) {
    (void)parent_context;
    if (!argv || !argv[0]) return 0;
    if (!strcmp(argv[0], "exit")) exit(0);
    if (!strcmp(argv[0], "cd")) {
        const char *dir = argv[1] ? argv[1] : getenv("HOME");
        if (!dir || chdir(dir) < 0) { perror("cd"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "help")) {
        puts("Built-ins: cd [directory], exit, help, jobs");
        return 0;
    }
    if (!strcmp(argv[0], "jobs")) {
        print_jobs();
        return 0;
    }
    if (!strcmp(argv[0], "history")) { print_history(); return 0; }
    return -1;
}

static char *history_entries[HISTORY_SIZE];
static unsigned long history_total;

void remember_command(const char *line) {
    if (!line || !*line) return;
    unsigned long slot = history_total % HISTORY_SIZE;
    free(history_entries[slot]);
    history_entries[slot] = strdup(line);
    ++history_total;
    add_history(line);
}

void print_history(void) {
    unsigned long first = history_total > HISTORY_SIZE ? history_total - HISTORY_SIZE : 0;
    for (unsigned long n = first; n < history_total; ++n) {
        const char *line = history_entries[n % HISTORY_SIZE];
        if (line) printf("%lu  %s\n", n + 1, line);
    }
}

char *history_command(const char *line) {
    if (!line || line[0] != '!') return NULL;
    char *end = NULL;
    errno = 0;
    unsigned long n = strtoul(line + 1, &end, 10);
    if (errno || end == line + 1 || *end || n == 0 || n > history_total ||
        n <= history_total - (history_total > HISTORY_SIZE ? HISTORY_SIZE : history_total)) {
        fprintf(stderr, "history: event not found: %s\n", line);
        return strdup("");
    }
    const char *entry = history_entries[(n - 1) % HISTORY_SIZE];
    if (!entry) { fprintf(stderr, "history: event not found: %s\n", line); return strdup(""); }
    puts(entry);
    return strdup(entry);
}
