#include "shell.h"

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
        if (*line) execute_line(line);
        free(line);
    }
    puts("\nShell exited.");
    return 0;
}
