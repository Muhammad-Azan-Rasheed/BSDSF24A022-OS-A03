#include "shell.h"

int main(void) {
    char *line;
    while ((line = read_cmd(PROMPT)) != NULL) {
        char **argv = tokenize(line);
        if (argv && argv[0]) {
            if (is_builtin(argv[0])) run_builtin(argv, true);
            else execute_tokens(argv, line, false);
        }
        free_tokens(argv);
        free(line);
    }
    puts("\nShell exited.");
    return 0;
}
