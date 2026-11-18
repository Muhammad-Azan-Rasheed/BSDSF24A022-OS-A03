#ifndef SHELL_H
#define SHELL_H

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define HISTORY_SIZE 20
#define MAX_JOBS 128
#define PROMPT "myshell> "

typedef struct Job {
    pid_t pid;
    char *command;
    struct Job *next;
} Job;

typedef struct Variable {
    char *name;
    char *value;
    struct Variable *next;
} Variable;

char **tokenize(const char *line);
void free_tokens(char **tokens);
char *read_cmd(const char *prompt);
int execute_line(const char *line);
int execute_tokens(char **tokens, const char *display, bool background);
int run_builtin(char **argv, bool parent_context);
bool is_builtin(const char *name);
void reap_jobs(void);
void print_jobs(void);
void set_variable(const char *name, const char *value);
const char *get_variable(const char *name);
void print_variables(void);
void remember_command(const char *line);
void print_history(void);
char *history_command(const char *line);

#endif
