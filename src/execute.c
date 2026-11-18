#include "shell.h"

typedef struct {
    char **argv;
    char *input;
    char *output;
} Stage;

static Job *jobs;
static size_t job_count;

static void add_job(pid_t pid, const char *command) {
    if (job_count >= MAX_JOBS) { fprintf(stderr, "jobs: job table full\n"); return; }
    Job *job = calloc(1, sizeof(*job));
    if (!job) return;
    job->pid = pid;
    job->command = strdup(command ? command : "");
    job->next = jobs;
    jobs = job;
    ++job_count;
    printf("[%ld] %ld\n", (long)job_count, (long)pid);
}

void reap_jobs(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        Job **link = &jobs;
        while (*link) {
            if ((*link)->pid == pid) {
                Job *done = *link;
                *link = done->next;
                free(done->command);
                free(done);
                if (job_count) --job_count;
                break;
            }
            link = &(*link)->next;
        }
    }
}

void print_jobs(void) {
    reap_jobs();
    for (Job *job = jobs; job; job = job->next)
        printf("[%ld] Running %s\n", (long)job->pid, job->command);
}

static int apply_redirection(Stage *stage) {
    if (stage->input) {
        int fd = open(stage->input, O_RDONLY);
        if (fd < 0 || dup2(fd, STDIN_FILENO) < 0) { perror(stage->input); if (fd >= 0) close(fd); return -1; }
        close(fd);
    }
    if (stage->output) {
        int fd = open(stage->output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0) { perror(stage->output); if (fd >= 0) close(fd); return -1; }
        close(fd);
    }
    return 0;
}

static int parse_stages(char **tokens, size_t begin, size_t end, Stage **out, size_t *stage_count) {
    size_t n = 1;
    for (size_t i = begin; i < end; ++i) if (!strcmp(tokens[i], "|")) ++n;
    Stage *stages = calloc(n, sizeof(*stages));
    if (!stages) return -1;
    size_t si = 0, argc = 0, cap = 8;
    stages[0].argv = calloc(cap, sizeof(char *));
    if (!stages[0].argv) { free(stages); return -1; }
    for (size_t i = begin; i <= end; ++i) {
        if (i == end || !strcmp(tokens[i], "|")) {
            if (!argc) { fprintf(stderr, "syntax error: empty pipeline stage\n"); goto bad; }
            stages[si].argv[argc] = NULL;
            if (i < end) {
                ++si; argc = 0; cap = 8;
                stages[si].argv = calloc(cap, sizeof(char *));
                if (!stages[si].argv) goto bad;
            }
            continue;
        }
        if (!strcmp(tokens[i], "<") || !strcmp(tokens[i], ">")) {
            bool input = !strcmp(tokens[i], "<");
            if (i + 1 >= end || strchr("|;&<>", tokens[i + 1][0])) { fprintf(stderr, "syntax error: expected filename after %s\n", tokens[i]); goto bad; }
            char **slot = input ? &stages[si].input : &stages[si].output;
            free(*slot);
            *slot = strdup(tokens[++i]);
            continue;
        }
        if (argc + 1 >= cap) { cap *= 2; char **g = realloc(stages[si].argv, cap * sizeof(char *)); if (!g) goto bad; stages[si].argv = g; }
        stages[si].argv[argc++] = strdup(tokens[i]);
    }
    *out = stages;
    *stage_count = n;
    return 0;
bad:
    for (size_t j = 0; j < n; ++j) {
        free_tokens(stages[j].argv);
        free(stages[j].input);
        free(stages[j].output);
    }
    free(stages);
    return -1;
}

static void free_stages(Stage *stages, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        free_tokens(stages[i].argv);
        free(stages[i].input);
        free(stages[i].output);
    }
    free(stages);
}

static int execute_segment(char **tokens, size_t begin, size_t end, const char *display, bool background) {
    if (begin >= end) return 0;
    Stage *stages = NULL;
    size_t n = 0;
    if (parse_stages(tokens, begin, end, &stages, &n) < 0) return 2;

    if (n == 1 && is_builtin(stages[0].argv[0]) && !background) {
        int saved_in = dup(STDIN_FILENO), saved_out = dup(STDOUT_FILENO);
        int rc = apply_redirection(&stages[0]) < 0 ? 1 : run_builtin(stages[0].argv, true);
        if (saved_in >= 0) { dup2(saved_in, STDIN_FILENO); close(saved_in); }
        if (saved_out >= 0) { dup2(saved_out, STDOUT_FILENO); close(saved_out); }
        free_stages(stages, n);
        return rc < 0 ? 0 : rc;
    }

    pid_t *pids = calloc(n, sizeof(*pids));
    int previous = -1;
    size_t started = 0;
    for (size_t i = 0; i < n; ++i) {
        int pipefd[2] = {-1, -1};
        if (i + 1 < n && pipe(pipefd) < 0) { perror("pipe"); break; }
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); if (pipefd[0] >= 0) close(pipefd[0]); if (pipefd[1] >= 0) close(pipefd[1]); break; }
        if (pid == 0) {
            if (previous >= 0) { dup2(previous, STDIN_FILENO); close(previous); }
            if (pipefd[1] >= 0) { dup2(pipefd[1], STDOUT_FILENO); close(pipefd[1]); }
            if (pipefd[0] >= 0) close(pipefd[0]);
            if (apply_redirection(&stages[i]) < 0) _exit(1);
            if (is_builtin(stages[i].argv[0])) _exit(run_builtin(stages[i].argv, false));
            execvp(stages[i].argv[0], stages[i].argv);
            perror(stages[i].argv[0]);
            _exit(127);
        }
        pids[started++] = pid;
        if (previous >= 0) close(previous);
        if (pipefd[1] >= 0) close(pipefd[1]);
        previous = pipefd[0];
    }
    if (previous >= 0) close(previous);
    int result = 0;
    if (background) {
        if (started) add_job(pids[started - 1], display);
    } else {
        for (size_t i = 0; i < started; ++i) {
            int status;
            while (waitpid(pids[i], &status, 0) < 0 && errno == EINTR) { }
            if (i + 1 == started && WIFEXITED(status)) result = WEXITSTATUS(status);
            else if (i + 1 == started && WIFSIGNALED(status)) result = 128 + WTERMSIG(status);
        }
    }
    free(pids);
    free_stages(stages, n);
    return result;
}

int execute_tokens(char **argv, const char *display, bool background) {
    size_t count = 0;
    while (argv && argv[count]) ++count;
    return execute_segment(argv, 0, count, display, background);
}

int execute_line(const char *line) {
    char **tokens = tokenize(line);
    if (!tokens) return 2;
    size_t count = 0;
    while (tokens[count]) ++count;
    size_t start = 0;
    int result = 0;
    for (size_t i = 0; i <= count; ++i) {
        if (i == count || !strcmp(tokens[i], ";")) {
            size_t end = i;
            bool background = end > start && !strcmp(tokens[end - 1], "&");
            if (background) --end;
            if (end > start) {
                char *display = strdup(line);
                result = execute_segment(tokens, start, end, display ? display : line, background);
                free(display);
            }
            start = i + 1;
        }
    }
    free_tokens(tokens);
    return result;
}
