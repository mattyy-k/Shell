anax-shell

A full Unix-like shell written from scratch in C++.

This project is a ground-up implementation of a functioning shell, featuring its own parser, execution engine, pipeline subsystem, history engine, tab completion, I/O redirection system, and process model.

No libraries except POSIX and GNU Readline.

Features
Command Parsing & Tokenization

Full tokenizer supporting:

Single quotes '...'

Double quotes "..." with correct escape semantics

Backslash escaping (Bash-compatible)

Multiline input continuation

Handles whitespace, tabs, and complex shell edge cases.

Built-in Commands

cd

exit

echo

pwd

type

history (with -r, -w, -a)

I/O Redirection

Supports:

> and 1> (overwrite stdout)

>> and 1>> (append stdout)

2> (stderr redirection)

2>> (stderr append)

Implemented via:

open()

dup2()

Proper file mode handling

Pipelines

Supports:

Two-stage pipelines (cmd1 | cmd2)

Multi-stage pipelines (cmd1 | cmd2 | cmd3 | ...)

Builtins inside pipelines (echo hi | wc)

Internals:

Dynamic pipe array

N-child fork loop

Correct closing of unused pipe ends

Final process cleanup via waitpid

History Engine

In-memory tracking of every command

history -r <file> → load

history -w <file> → overwrite-save

history -a <file> → append new entries only

HISTFILE environment variable support

Auto-save on exit

Tab Completion

Using GNU Readline:

Autocompletes builtins (echo, exit, etc.)

Autocompletes executables from $PATH

Custom generator function

Process Model

Fork/exec execution engine

Distinguishes between builtins and external programs

Parent-only exit

Proper environment inheritance

Architecture Overview
┌────────────┐
│   Input    │  ← readline()
└─────┬──────┘
      │
┌─────▼──────┐
│ Tokenizer  │  ← quotes, escapes, multiline
└─────┬──────┘
      │
┌─────▼──────┐
│  Parser    │  ← pipes, redirects, builtins
└─────┬──────┘
      │
┌─────▼──────────────────────────┐
│ Execution Engine               │
│ - Builtins                     │
│ - Fork/exec external commands  │
│ - Redirects                    │
│ - Pipelines                    │
└───────────────────────────────┘

Build & Run
Requirements

C++17 or later

GNU Readline

Linux/macOS/WSL

Build
g++ -std=gnu++17 -lreadline main.cpp -o shell

Run
./shell

Example Usage
$ echo hello world
hello world

$ echo a b c > out.txt
$ cat out.txt
a b c

$ echo hi | wc -c
3

$ ls | type exit
exit is a shell builtin

$ history -a history.txt

What I Learned

POSIX process control (fork, execvp, waitpid)

File descriptor manipulation (dup2, open)

Pipe-based IPC

Mini language parsing design

Handling shell quoting and escaping rules

Building a REPL

Readline integration

Implementing real shell semantics

Planned Upgrades

Job control (fg/bg/& and Ctrl+Z)

Shell scripting (variables, loops, conditionals)

Terminal emulator frontend

Plugin architecture

Better error messages

Why This Project Matters

This shell is a real systems project demonstrating understanding of:

Low-level Linux APIs

Processes, IPC, and piping

Tokenization and parsing

File descriptors and redirection

Unix shell semantics

Readline integration

Anyone reading this repository can immediately see depth in OS-level engineering.
