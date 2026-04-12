# Custom UNIX Utilities

A collection of 7 lightweight UNIX command-line utilities implemented from scratch in pure C (C11). These are simplified reimplementations of standard UNIX commands, prefixed with `custom_` to avoid conflicts with system binaries.

---

## Utilities

| Utility | Replaces | Description |
|---|---|---|
| `custom_ls` | `ls` | List directory contents |
| `custom_cat` | `cat` | Concatenate and display file content |
| `custom_grep` | `grep` | Search for patterns in files |
| `custom_wc` | `wc` | Count words, lines, and bytes |
| `custom_cp` | `cp` | Copy files and directories |
| `custom_mv` | `mv` | Move or rename files |
| `custom_rm` | `rm` | Remove files and directories |

---

## Project Structure

```
unix_utils/
├── Makefile
└── src/
    ├── custom_ls.c
    ├── custom_cat.c
    ├── custom_grep.c
    ├── custom_wc.c
    ├── custom_cp.c
    ├── custom_mv.c
    └── custom_rm.c
```

---

## Getting Started

### Prerequisites

- `gcc` or any C11-compatible compiler
- `make`
- Standard POSIX C library

### Build

```bash
git clone https://github.com/yourusername/unix_utils.git
cd unix_utils
make all
```

### Install

```bash
sudo make install
```

Installs all binaries to `/usr/local/bin`, so you can run `custom_ls`, `custom_grep`, etc. from anywhere without a `./` prefix.

### Uninstall

```bash
sudo make uninstall
```

### Clean Build Artifacts

```bash
make clean
```

---

## Usage

### custom_ls
```bash
custom_ls [OPTIONS] [path...]

  -l    Long format (permissions, owner, size, date)
  -a    Show all files including hidden files
  -h    Human-readable sizes (use with -l)
  -r    Reverse sort order
  -t    Sort by modification time
  -1    One entry per line
```

### custom_cat
```bash
custom_cat [OPTIONS] [file...]

  -n    Number all output lines
  -b    Number non-blank lines only
  -s    Squeeze multiple blank lines into one
  -E    Show $ at end of each line
  -T    Show tabs as ^I
  -v    Show non-printing characters
  -A    Equivalent to -vET
```

### custom_grep
```bash
custom_grep [OPTIONS] pattern [file...]

  -i    Case-insensitive matching
  -v    Invert match
  -n    Show line numbers
  -c    Print match count only
  -l    List filenames with matches only
  -r    Recursive search
  -w    Match whole words only
  -x    Match whole lines only
  -q    Quiet mode (exit code only)
  -e    Specify pattern explicitly
  -H    Always print filename
  -h    Never print filename
```

### custom_wc
```bash
custom_wc [OPTIONS] [file...]

  -l    Line count
  -w    Word count
  -c    Byte count
  -m    Character count
  -L    Length of longest line

# Default (no flags): prints lines, words, and bytes
```

### custom_cp
```bash
custom_cp [OPTIONS] source... destination

  -r/-R   Copy directories recursively
  -v      Verbose output
  -f      Force overwrite
  -p      Preserve timestamps and permissions
  -n      Do not overwrite existing files
  -u      Copy only if source is newer
```

### custom_mv
```bash
custom_mv [OPTIONS] source... destination

  -v    Verbose output
  -f    Force overwrite
  -n    Do not overwrite existing files
  -i    Prompt before overwriting
  -u    Move only if source is newer
```

### custom_rm
```bash
custom_rm [OPTIONS] file...

  -r/-R   Remove directories recursively
  -f      Force, ignore nonexistent files
  -v      Verbose output
  -i      Prompt before every removal
  -d      Remove empty directories
```

---

## Examples

```bash
# List all files with details and human-readable sizes
custom_ls -lah

# Display a file with line numbers
custom_cat -n main.c

# Recursively search for TODO comments
custom_grep -rn "TODO" ./src

# Count lines across all C files
custom_wc -l src/*.c

# Copy a directory tree
custom_cp -rv src/ backup/

# Move multiple files to a directory
custom_mv -v *.log /var/archive/

# Force delete a build directory
custom_rm -rf build/
```

---

## Compatibility

Compatible with Linux and other UNIX-like systems. Compiled with `-std=c11 -D_DEFAULT_SOURCE`.

---

## License

MIT