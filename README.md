# BSDSF24M002-OS-A02: A Custom `ls` Clone in C

A Unix `ls` utility re-engineered from scratch in C for the Operating Systems course (Assignment 02). Starting from a bare-bones directory lister, it was extended feature by feature, each on its own Git branch and released as a tagged version.

## Features

| Version | Feature | Flag |
|---------|---------|------|
| v1.1.0 | Long listing: permissions, links, owner, group, size, date, symlink targets | `-l` |
| v1.2.0 | Column display, "down then across", adapts to terminal width | default |
| v1.3.0 | Horizontal display, left to right | `-x` |
| v1.4.0 | Alphabetical sorting (case-insensitive) | default |
| v1.5.0 | Colorized output by file type | default |
| v1.6.0 | Recursive directory listing | `-R` |

Flags can be combined, for example `-lR` or `-xR`. When `-l` and `-x` are used together, `-l` takes priority.

## Color Scheme

| Color | File type |
|-------|-----------|
| Blue | Directories |
| Green | Executable files |
| Red | Archives (`.tar`, `.gz`, `.zip`) |
| Pink | Symbolic links |
| Reverse video | Special files (devices, sockets, pipes) |

## Project Structure

```
BSDSF24M002-OS-A02/
├── src/
│   └── ls-v1.0.0.c    # source code (all features)
├── bin/               # compiled binary (ls)
├── obj/               # object files
├── man/
├── Makefile
├── REPORT.md          # answers to the report questions
└── README.md
```

## Build

Requires `gcc` and `make`.

```bash
git clone https://github.com/Haseeb0910/BSDSF24M002-OS-A02.git
cd BSDSF24M002-OS-A02
make
```

To remove the build output: `make clean`.

## Usage

```bash
./bin/ls                # column display of the current directory
./bin/ls /etc           # list a specific directory
./bin/ls -l             # long listing format
./bin/ls -x             # horizontal layout
./bin/ls -R             # recursive listing
./bin/ls -lR /tmp       # long format, recursive
```

Hidden files (names starting with `.`) are not shown.

## How It Works

- **Reading:** `opendir()`, `readdir()` and `closedir()` read the directory. Names are stored in a dynamic array that doubles in size with `realloc()` when full.
- **Sorting:** the array is sorted with `qsort()` and a custom comparator (case-insensitive, with `strcmp` as a tie-breaker).
- **Metadata:** `lstat()` is used instead of `stat()` so symbolic links are reported as links. Permissions are decoded from `st_mode` with bitwise masks, and `getpwuid()` and `getgrgid()` resolve owner and group names.
- **Layout:** `ioctl(TIOCGWINSZ)` gets the terminal width (falls back to 80 columns when output is piped). Rows and columns are computed from the longest filename.
- **Colors:** ANSI escape codes. Padding is added manually so the escape codes don't break column alignment.
- **Recursion:** `-R` calls the listing function on each subdirectory using the full path. Symbolic links are never followed, which prevents infinite loops.

## Releases

Each version has a tag and a GitHub release with the compiled binary attached: `v1.1.0` to `v1.6.0`. Every feature was developed on its own branch (`feature-<name>-vX.Y.Z`) and merged into `main`.

## Report

See [REPORT.md](REPORT.md) for the answers to the assignment questions.

## Author

Haseeb (BSDSF24M002)
