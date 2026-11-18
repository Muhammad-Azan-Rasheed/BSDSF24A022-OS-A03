# myshell

A small POSIX command interpreter written in C for PA-03. It demonstrates process creation, built-ins, command history, Readline editing and completion, redirection, pipelines, command chaining, background jobs, conditional blocks, and shell variables.

## Build and run

The project requires a POSIX environment, GCC or a compatible C compiler, GNU Make, and GNU Readline development headers/library.

```sh
make
./bin/myshell
```

Remove generated objects and the executable with `make clean`.

## Features

- Built-ins: `cd`, `exit`, `help`, `history`, `jobs`, and `set`.
- A 20-command history list with `!n` replay, plus Readline arrow-key history.
- Tab completion for executable names and filenames.
- `<` and `>` redirection, `|` pipelines, `;` sequential commands, and `&` background execution.
- A basic multiline `if command` / `then` / optional `else` / `fi` construct.
- Variable assignments such as `MESSAGE="Hello, World!"` and expansion using `$MESSAGE` or `${MESSAGE}`.

The parser intentionally implements a small teaching subset. It does not provide full POSIX shell behavior such as command substitution, globbing, or nested conditionals.
