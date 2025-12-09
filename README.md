# TinyShell — A Unix Shell Built From Scratch in C++

This project is a fully functional POSIX-style shell that I built from scratch in C++.

## Features

- Command execution using `fork()` + `execvp()`
- Builtins: `cd`, `pwd`, `echo`, `type`, `history`, `exit`
- Pipes (`|`) with unlimited stages
- Redirection (`>`, `>>`, `2>`, `2>>`)
- Persistent history file support (`history -r`, `-w`, `-a`)
- Tab autocomplete (builtins + executables from PATH)
- Multi-line quoting and custom tokenizer

## Build & Run

```bash
make
./local

## Next Planned features: 

-Job Control (fg, bg, jobs)

-Scripting support

-Terminal emulator
