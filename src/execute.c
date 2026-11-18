#include "shell.h"

int execute_tokens(char **argv, const char *display, bool background) {
    (void)display;
    (void)background;
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) { perror("waitpid"); return 1; }
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

int execute_line(const char *line) {
    char **argv = tokenize(line);
    if (!argv || !argv[0]) { free_tokens(argv); return 0; }
    int rc = is_builtin(argv[0]) ? run_builtin(argv, true) : execute_tokens(argv, line, false);
    free_tokens(argv);
    return rc < 0 ? 0 : rc;
}

void reap_jobs(void) { }
void print_jobs(void) { puts("Job control not yet implemented."); }
void set_variable(const char *name, const char *value) { (void)name; (void)value; }
const char *get_variable(const char *name) { (void)name; return NULL; }
void print_variables(void) { }
