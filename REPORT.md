# OS Assignment 02: Re-engineering the `ls` Utility — Report

**Name:** Haseeb
**Roll No:** BSDSF24M002
**Repository:** BSDSF24M002-OS-A02

## Version Summary

| Version | Feature | Branch |
|---------|---------|--------|
| v1.0.0 | Project setup, Makefile, starter code | `main` |
| v1.1.0 | Long listing (`-l`) | `feature-long-listing-v1.1.0` |
| v1.2.0 | Column display ("down then across") | `feature-column-display-v1.2.0` |
| v1.3.0 | Horizontal display (`-x`) | `feature-horizontal-display-v1.3.0` |
| v1.4.0 | Alphabetical sort | `feature-alphabetical-sort-v1.4.0` |
| v1.5.0 | Colorized output | `feature-colorized-output-v1.5.0` |
| v1.6.0 | Recursive listing (`-R`) | `feature-recursive-listing-v1.6.0` |

---

## Feature 2: Long Listing Format (v1.1.0)

### Q1. What is the difference between `stat()` and `lstat()`, and why is `lstat()` the right choice for `ls -l`?

Both system calls fill a `struct stat` with the metadata of a file (type, permissions, owner, size, timestamps). The difference is how they treat symbolic links. `stat()` follows the link and returns information about the file the link points to. `lstat()` does not follow it and returns information about the link itself.

`ls -l` must show a symbolic link as a link: type `l`, the size of the link, and its own timestamp, followed by `-> target`. If I used `stat()`, a link would look like the file or directory it points to, and a broken link would make `stat()` fail completely. So my `print_long()` uses `lstat()`, and then calls `readlink()` to get the target text.

### Q2. How do you extract the file type and permissions from the `st_mode` field?

`st_mode` is a bit field. Some of the bits store the file type and the lower bits store the permissions.

- **File type:** I use the macros `S_ISDIR()`, `S_ISLNK()`, `S_ISCHR()`, `S_ISBLK()`, `S_ISFIFO()` and `S_ISSOCK()`. Internally they mask `st_mode` with `S_IFMT` and compare the result with the matching type constant, for example `(st_mode & S_IFMT) == S_IFDIR`.
- **Permissions:** I test each bit with a bitwise AND against a mask. For example `st_mode & S_IRUSR` is non-zero when the owner has read permission, so that character is `'r'`; otherwise it is `'-'`. I repeat this for `S_IWUSR`, `S_IXUSR`, then the group masks (`S_IRGRP`, ...) and the others masks (`S_IROTH`, ...), which gives the nine characters like `rwxr-xr-x`.
- **Special bits:** `S_ISUID`, `S_ISGID` and `S_ISVTX` change the execute character to `s`/`S` or `t`/`T`.

---

## Feature 3: Column Display (v1.2.0)

### Q3. Explain the logic for printing items in a "down then across" columnar format. Why is a single loop not enough?

In "down then across", the items fill the first column from top to bottom, then the second column, and so on. But the terminal prints one whole row at a time, so the order in which I print is not the same as the order of the array. A single loop over the array would print the items left to right, which is the horizontal layout instead.

My logic is:
1. Find the longest name and set `col_width = maxlen + 2`.
2. Compute `cols = term_width / col_width` (at least 1).
3. Compute `rows = ceil(count / cols)`, written as `(count + cols - 1) / cols`.
4. Use a nested loop: the outer loop goes over rows `r`, the inner loop over columns `c`, and the item printed is `names[c * rows + r]`.

Example: 7 items, 3 columns gives 3 rows. Row 0 prints items 0, 3, 6; row 1 prints 1, 4; row 2 prints 2, 5. Because every row needs items from different parts of the array, I need the nested loop and the precomputed number of rows.

### Q4. What is the purpose of the `ioctl` system call here?

`ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)` asks the terminal driver for the current window size and stores it in a `struct winsize`. The field `ws_col` gives the number of character columns, which my code uses to decide how many columns of filenames fit on a line, so the layout adapts to the terminal width.

If the call fails, for example when the output is piped to a file or to another program (`ls | cat`), standard output is not a terminal and has no window size. In that case my code falls back to a width of 80 characters. A fixed width would be a poor choice because it would not adapt to a wider or narrower terminal.

---

## Feature 4: Horizontal Display (v1.3.0)

### Q5. How does your program handle the different display modes, and how does `-x` differ from the default layout?

The display mode is state kept in flags. `main()` uses `getopt()` to set `long_flag` and `horiz_flag`, and passes them to `do_ls()`. After all filenames are collected and sorted, `do_ls()` chooses one print function: `print_long()` if `-l` is set (so `-l` has priority over `-x`), `print_horizontal()` if `-x` is set, and `print_columns()` otherwise.

The two column layouts use the same column width and the same array. The difference is the print order. The default layout prints "down then across" with the index `c * rows + r`. The `-x` layout simply prints `names[0]`, `names[1]`, ... from left to right, tracks the current line position, and starts a new line when the next name would not fit in the terminal width. Since the names are already in an array, adding a new mode only needed one new print function.

---

## Feature 5: Alphabetical Sort (v1.4.0)

### Q6. Why is it necessary to read all directory entries into memory before sorting? What are the drawbacks of this approach for very large directories?

`readdir()` returns entries in an arbitrary order that depends on the file system, and it can only be read one entry at a time in one direction. To print the names in alphabetical order, I must know all of them first, because the entry that should be printed first could be the last one that `readdir()` returns. So I store every name in a dynamic array (grown with `realloc()` when full), sort it with `qsort()`, and only then print.

Drawbacks for very large directories:
- **Memory:** it needs O(n) memory, one string per entry, while printing immediately as entries are read would only need O(1) memory.
- **Time:** sorting costs O(n log n) comparisons on top of reading, and nothing can be shown until the whole directory is read.
- **Latency:** the user sees no output until the very end, which is slow for a directory with millions of files.

The benefit is a correct, sorted listing. The same array is also reused for the column layout, which needs the total count and the longest name before it can print anything.

### Q7. Explain the purpose and signature of the comparison function used by `qsort()`.

`qsort()` is a generic sorting function: it does not know the type of the elements, so I give it a function pointer to a comparator. The signature is `int compare(const void *a, const void *b)`. It must return a negative number if the first item should come first, zero if they are equal, and a positive number otherwise.

`qsort()` passes pointers to the array elements. My array elements are `char *`, so `a` and `b` are really `char **`. Inside the comparator I cast to `const char **` and dereference once to get the strings: `const char *s1 = *(const char **)a;`. I compare with `strcasecmp()` first (case-insensitive, like the real `ls`) and use `strcmp()` to break ties. Passing `strcmp` directly would be wrong because it would be given the addresses of the pointers instead of the strings.

---

## Feature 6: Colorized Output (v1.5.0)

### Q8. How do ANSI escape codes work, and why can't you use `printf("%-*s")` to align colored names?

ANSI escape codes are special byte sequences that the terminal interprets as formatting commands instead of printing them. For example `\033[0;34m` switches the text color to blue, `\033[0;32m` to green, `\033[0;31m` to red, `\033[0;35m` to pink/magenta, `\033[7m` to reverse video, and `\033[0m` resets everything. I print the color code, then the name, then the reset code.

The problem with alignment is that `printf("%-*s", width, str)` pads based on the number of characters in the string, and the escape bytes count as characters even though they take no space on the screen. A colored name would get less padding than it needs, and the columns would be crooked. To fix this, my `print_name()` prints the colored text first and then adds the spaces manually, using `strlen()` of the plain filename only: `pad = width - strlen(name)`.

### Q9. How does your program detect each file type and whether a file is executable?

For every name, `get_color()` builds the full path and calls `lstat()`. Then it checks `st_mode`:
- `S_ISLNK` is a symbolic link (pink),
- `S_ISDIR` is a directory (blue),
- `S_ISCHR`, `S_ISBLK`, `S_ISSOCK` or `S_ISFIFO` is a special file (reverse video),
- `st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)` is non-zero for an executable file (green), meaning at least one of the user, group or other execute bits is set,
- a name ending in `.tar`, `.gz` or `.zip` (checked with my helper `has_suffix()`) is an archive (red).

The symbolic link check comes first so that a link is shown as a link, not as the thing it points to.

---

## Feature 7: Recursive Listing (v1.6.0)

### Q10. What is the base case in your recursive function, and how does it stop?

The function `do_ls()` calls itself for each subdirectory when `-R` is set. The base case is a directory that contains no subdirectories: after printing its entries, the loop that looks for subdirectories finds nothing to recurse into, so the function simply returns. A second stopping case is when `opendir()` fails, for example because of missing permission; the function prints an error and returns immediately.

The recursion always terminates because each call goes one level deeper into a finite directory tree, and because symbolic links are not followed, so the traversal cannot loop back to a parent directory.

### Q11. Why use `lstat()` instead of `stat()` when deciding whether to recurse?

If I used `stat()`, a symbolic link that points to a directory would look like a directory, and my code would recurse into it. A link that points to one of its own parent directories (I tested with `/tmp/r/a/loop -> /tmp/r`) would then be followed forever, creating an infinite recursion until the path becomes too long or the program crashes. A link could also lead outside the tree being listed.

With `lstat()`, such a link is reported as a link (`S_ISLNK`) and not as a directory (`S_ISDIR`), so it is listed but never entered. The names `.` and `..` are also never visited, because my code skips every entry whose name starts with a dot.

### Q12. How do you build the path of a subdirectory, and why is the bare filename not enough?

I build it with `snprintf(path, sizeof(path), "%s/%s", dir, names[i])`, which joins the current directory and the entry name, for example `/tmp/r` + `a` gives `/tmp/r/a`. This full path is passed to `lstat()` and to the recursive `do_ls()` call, and it is also used for the heading `path:` that is printed before each directory.

`readdir()` only returns the name of the entry, not its location. System calls like `lstat()` and `opendir()` resolve relative names against the current working directory of the process, not against the directory being listed. If I passed only `a`, the program would look for `a` in the directory where I ran it, and it would fail or, worse, find a different file. The same reason applies to `print_long()` and `get_color()`, which also build the full path before calling `lstat()`.

---

## Build and Usage

```bash
make
./bin/ls              # column display, sorted, colored
./bin/ls -l           # long listing
./bin/ls -x           # horizontal display
./bin/ls -R           # recursive
./bin/ls -lR /etc     # long format, recursive
```
