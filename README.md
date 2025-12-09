# anax-shell  
*A full Unix-like shell written from scratch in C++.*

This project is a ground-up implementation of a functioning shell, featuring its own parser, execution engine, pipeline subsystem, history engine, tab completion, I/O redirection system, and process model.

No libraries except **POSIX** and **GNU Readline**.

---

## ⚙️ Features

### 🧩 Command Parsing & Tokenization
- Full tokenizer supporting:
  - Single quotes `'...'`
  - Double quotes `"..."` with escape semantics
  - Backslash escaping (Bash-compatible)
  - Multiline input continuation
- Handles whitespace, tabs, and tricky shell edge cases.

---

### 📦 Built-in Commands
- `cd`
- `exit`
- `echo`
- `pwd`
- `type`
- `history` (`-r`, `-w`, `-a`)

---

### 🔁 I/O Redirection
Supports:
- `>` and `1>` — overwrite stdout  
- `>>` and `1>>` — append stdout  
- `2>` — redirect stderr  
- `2>>` — append stderr  

Implemented manually using:
- `open()`
- `dup2()`
- Mode flags (O_TRUNC, O_APPEND, O_CREAT, etc.)

---

### 🚰 Pipelines
Supports:
- Two-stage: `cmd1 | cmd2`
- Multi-stage: `cmd1 | cmd2 | cmd3 | ...`
- Builtins inside pipelines: `echo hi | wc`

Pipeline internals:
- Dynamic `pipe[N]` allocation  
- N-child fork loop  
- Proper closing of unused pipe ends  
- Cleanup via `waitpid()`  

---

### 🕘 History Engine
- Stores every command in memory  
- `history -r <file>` → load  
- `history -w <file>` → write  
- `history -a <file>` → append (new entries only)
- Honors the `HISTFILE` environment variable  
- Auto-save on exit  

---

### 🔮 Tab Completion (Readline)
- Autocompletes builtins
- Autocompletes executables from `$PATH`
- Custom generator function using Readline APIs

---

### 👥 Process Model
- Full fork/exec execution engine
- Correct handling of builtins vs external programs
- Parent-only exit
- Correct environment + FD inheritance  

---

## 🛠️ Build & Run

### Requirements
- **C++17**
- **GNU Readline**
- Linux / macOS / WSL

### Build

---

## 🛠️ Build & Run

### Requirements
- **C++17**
- **GNU Readline**
- Linux / macOS / WSL

### Build

g++ -std=gnu++17 -lreadline main.cpp -o shell

###RUN
./shell


---

## 📜 Example Usage

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


---

## 📚 What I Learned
- POSIX process control (`fork`, `execvp`, `waitpid`)
- File descriptor manipulation (`open`, `dup2`)
- Pipe-based IPC
- Mini-language parser design
- Handling quoting and escaping
- Readline integration
- Shell architecture fundamentals

---

## 🚀 Planned Upgrades
- Job control (`fg`, `bg`, `&`, Ctrl+Z)
- Shell scripting (variables, loops, conditionals)
- Terminal emulator frontend
- Plugin / extension architecture
- Better diagnostics

---

## 🧠 Why This Project Matters
This project demonstrates strong systems-level understanding:

- Low-level Linux APIs  
- Process lifecycle management  
- IPC and pipelines  
- Tokenization and parsing  
- Real shell semantics  
- Readline-based UX  


---
