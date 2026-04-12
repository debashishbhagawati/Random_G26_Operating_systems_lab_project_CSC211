# CSC211 — Operating Systems Lab
## Group Name: Random (G26)

---

## 👥 Group Members

| S.No | Name | Admission No. |
|------|------|---------------|
| 1 | Arpit Singh | 22JE0173 |
| 2 | Debashish Bhagawati | 22JE0297 |
| 3 | Zafeer Alam | 22JE0582 |
| 4 | Alapana Durga Rao | 22JE0088 |
| 5 | Jyani Kushal | 22JE0442 |
| 6 | Pavan Kumar | 22JE0249 |

---

## 📁 Repository Structure

```
CSC211-OS-Lab/
├── project1_xv6_syscalls/       # Project 1: xv6 Custom System Calls
│   ├── kernel/
│   │   ├── syscall.h            # Syscall number definitions
│   │   ├── syscall.c            # Syscall dispatch table
│   │   ├── proc.h               # Process struct + pinfo struct
│   │   └── sysproc_custom.c     # Custom syscall implementations
│   ├── user/
│   │   ├── user.h               # User-space function prototypes
│   │   ├── usys.pl              # Syscall entry stubs
│   │   └── syscall_demo.c       # Demo user program
│   └── Makefile
│
└── project2_unix_utilities/     # Project 2: Custom UNIX Utilities
    ├── custom_ls.c
    ├── custom_cat.c
    ├── custom_grep.c
    ├── custom_wc.c
    ├── custom_cp.c
    ├── custom_mv.c
    ├── custom_rm.c
    └── Makefile
```

---

# Project 1: xv6 Custom System Calls

## Overview

This project extends the [xv6 RISC-V operating system](https://github.com/mit-pdos/xv6-riscv) with **6 new system calls** covering process inspection, inter-process communication (IPC), threads, locks, and signals.

## System Calls Implemented

| # | Syscall | Number | Functionality |
|---|---------|--------|---------------|
| 1 | `getpinfo()` | 22 | Retrieve process info (PID, PPID, state, name) |
| 2 | `pipe2()` | 23 | Enhanced pipe creation with flags (IPC) |
| 3 | `thread_create()` | 24 | Lightweight thread creation with shared memory |
| 4 | `mutex_lock()` | 25 | Acquire a user-space mutex lock |
| 5 | `mutex_unlock()` | 26 | Release a user-space mutex lock |
| 6 | `sendsig()` | 27 | Send signals between processes (SIGTERM/STOP/CONT) |

## Files Modified / Created

| File | Action | Purpose |
|------|--------|---------|
| `kernel/syscall.h` | Modified | Added syscall numbers 22–27 |
| `kernel/syscall.c` | Modified | Added externs and dispatch table entries |
| `kernel/proc.h` | Modified | Added `pending_sig`, `sig_handler` fields and `struct pinfo` |
| `kernel/sysproc_custom.c` | **New** | All 6 syscall implementations |
| `user/usys.pl` | Modified | Added 6 `entry()` stubs |
| `user/user.h` | Modified | Added `struct pinfo` and 6 function prototypes |
| `user/syscall_demo.c` | **New** | User-space demo program |
| `Makefile` | Modified | Added new object file and user program |

## API Reference

### `getpinfo(int pid, struct pinfo *info)`
Fills a `struct pinfo` with information about the process identified by `pid`.
- **Returns:** `0` on success, `-1` if PID not found
- **Fields:** `pid`, `ppid`, `state` (0=UNUSED … 4=ZOMBIE), `name[16]`

### `pipe2(int pipefd[2], int flags)`
Creates a pipe and writes read/write file descriptors into `pipefd`.
- **flags:** `1` = non-blocking read end marker
- **Returns:** `0` on success, `-1` on failure

### `thread_create(void (*fn)(void*), void *arg, void *stack)`
Spawns a new thread (process sharing parent's page table) starting at function `fn` with argument `arg` on the provided `stack`.
- **Returns:** Thread TID (PID) on success, `-1` on failure

### `mutex_lock(int *mutex)` / `mutex_unlock(int *mutex)`
Acquires/releases a mutex stored at a user-space integer address (`0` = free, `1` = locked). `mutex_lock` yields the CPU while waiting.
- **Returns:** `0` on success, `-1` on failure

## Build & Run

```bash
# Inside your xv6 directory
make clean
make qemu

# Inside the xv6 shell
$ syscall_demo
```

## Expected Output

```
=== xv6 Custom Syscall Demo ===

--- Demo 1: getpinfo (Process Info) ---
  PID:   3
  PPID:  2
  State: 3 (3=RUNNING)
  Name:  syscall_demo

--- Demo 2: pipe2 (IPC Pipe with Flags) ---
  pipe2 created: read_fd=3, write_fd=4
  Parent received: "Hello via pipe2!"

--- Demo 3: thread_create (Threads) ---
  Thread created with TID=4
  [Thread] Hello from thread! arg = 42
  [Thread] My PID = 4

--- Demo 4: mutex_lock/unlock (Locks) ---
  Started lock workers TID=5 and TID=6
  Final shared_counter = 10 (expected 10)

=== All demos complete! ===
```

---

# Project 2: Custom UNIX Utilities

## Overview

Seven core UNIX command-line utilities re-implemented from scratch in **C (C11 standard)**, demonstrating low-level system calls, file I/O, and directory handling in a POSIX-compliant environment. All utilities are prefixed with `custom_` to avoid conflicts with system binaries.

## Technical Specifications

| Item | Detail |
|------|--------|
| Language | C (C11 Standard) |
| Compiler | GCC |
| Build System | GNU Make |
| Libraries | `stdio.h`, `stdlib.h`, `unistd.h`, `dirent.h`, `sys/stat.h`, `string.h` |

## Utilities

### `custom_ls` — Directory Listing
Lists files and directories with support for permissions, hidden files, and sorting.

| Flag | Description |
|------|-------------|
| `-l` | Long format (permissions, owner, size, date) |
| `-a` | Show hidden files (dotfiles) |
| `-h` | Human-readable file sizes |
| `-r` | Reverse sort order |
| `-t` | Sort by modification time |

```bash
custom_ls -lah
custom_ls -lrt
```

---

### `custom_cat` — File Concatenation & Display
Reads files and outputs contents to stdout with optional formatting.

| Flag | Description |
|------|-------------|
| `-n` | Number all output lines |
| `-A` | Show tabs as `^I` and line ends as `$` |

```bash
custom_cat -nsA fileA.txt
custom_cat fileB.txt fileA.txt
```

---

### `custom_grep` — Pattern Matching
Searches for a string pattern within files or directories.

| Flag | Description |
|------|-------------|
| `-i` | Case-insensitive matching |
| `-r` | Recursive directory traversal |
| `-n` | Show line numbers |

```bash
custom_grep -in "unix" fileA.txt
custom_grep -rn "TODO" .
```

---

### `custom_wc` — Word/Line/Byte Counter
Counts lines, words, and bytes in one or more files.

| Flag | Description |
|------|-------------|
| (default) | Lines, words, bytes |
| `-L` | Length of the longest line |

```bash
custom_wc fileA.txt fileB.txt
custom_wc -L fileA.txt
```

---

### `custom_cp` — File Copy
Copies files and directories.

| Flag | Description |
|------|-------------|
| `-v` | Verbose output |
| `-r` | Recursive directory copy |

```bash
custom_cp -vp fileA.txt fileA_backup.txt
custom_cp -rv nested_dir backup_dir
```

---

### `custom_mv` — File Move / Rename
Moves or renames files and directories.

| Flag | Description |
|------|-------------|
| `-v` | Verbose output |
| `-i` | Interactive (prompt before overwrite) |

```bash
custom_mv -v fileB.txt renamed_fileB.txt
custom_mv -iv fileA_backup.txt renamed_fileB.txt
```

---

### `custom_rm` — File Removal
Deletes files and directories securely.

| Flag | Description |
|------|-------------|
| `-r` | Recursive deletion |
| `-f` | Force (no prompt) |
| `-v` | Verbose output |
| `-i` | Interactive (prompt before each deletion) |

```bash
custom_rm -i renamed_fileB.txt
custom_rm -rfv backup_dir
```

---

## Build Instructions

```bash
# Compile all utilities
make all

# Install to /usr/local/bin (requires sudo)
sudo make install

# Clean build artifacts
make clean
```

---

## Dependencies

**Project 1 (xv6):**
- QEMU emulator
- RISC-V GCC cross-compiler (`riscv64-unknown-elf-gcc`)
- GNU Make

**Project 2 (UNIX Utilities):**
- GCC (any modern version supporting C11)
- GNU Make
- POSIX-compliant Linux/macOS environment

---

## License

This project was developed for academic purposes as part of the **Operating Systems Lab (CSC211)** course.

---

*Group Random (G26) — CSC211 Operating Systems Lab*
