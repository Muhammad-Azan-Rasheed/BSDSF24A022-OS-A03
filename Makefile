CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11 -g -Iinclude
LDFLAGS ?=
LDLIBS ?= -lreadline
TARGET := bin/myshell
SOURCES := src/main.c src/shell.c src/execute.c
OBJECTS := $(SOURCES:src/%.c=obj/%.o)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS) | bin
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

obj/%.o: src/%.c include/shell.h | obj
	$(CC) $(CFLAGS) -c $< -o $@

bin obj:
	mkdir -p $@

clean:
	rm -rf bin obj
