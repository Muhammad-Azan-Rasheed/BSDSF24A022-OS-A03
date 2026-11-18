#include "shell.h"

char *read_cmd(const char *prompt) {
    fputs(prompt, stdout);
    fflush(stdout);
    char *line = NULL;
    size_t cap = 0;
    ssize_t n = getline(&line, &cap, stdin);
    if (n < 0) { free(line); return NULL; }
    if (n && line[n - 1] == '\n') line[n - 1] = '\0';
    return line;
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
                    !strcmp(name, "help") || !strcmp(name, "jobs"));
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
        puts("Job control not yet implemented.");
        return 0;
    }
    return -1;
}
