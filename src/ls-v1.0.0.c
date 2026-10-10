/*
 * Programming Assignment 02: ls-v1.3.0
 * Features: -l (long listing), -x (horizontal), default column display
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <strings.h>

extern int errno;

void do_ls(const char *dir, int long_flag, int horiz_flag);
void print_long(const char *dir, const char *name);
void print_columns(char **names, int count);
void print_horizontal(char **names, int count);
int compare_names(const void *a, const void *b);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;
    int horiz_flag = 0;

    // getopt scans argv for options like -l and -x
    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        case 'x':
            horiz_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [dir...]\n", argv[0]);
            exit(1);
        }
    }

    // optind = index of first non-option argument (a directory name)
    if (optind == argc)
    {
        do_ls(".", long_flag, horiz_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s :\n", argv[i]);
            do_ls(argv[i], long_flag, horiz_flag);
            puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir, int long_flag, int horiz_flag)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    // Dynamic array of filenames: grows as needed
    int capacity = 16;
    int count = 0;
    char **names = malloc(capacity * sizeof(char *));
    if (names == NULL)
    {
        perror("malloc");
        closedir(dp);
        return;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        // Array full? Double its size
        if (count == capacity)
        {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (tmp == NULL)
            {
                perror("realloc");
                break;
            }
            names = tmp;
        }
        names[count++] = strdup(entry->d_name);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }
    closedir(dp);

    qsort(names, count, sizeof(char *), compare_names);

    // Display: -l has priority over -x
    if (long_flag)
    {
        for (int i = 0; i < count; i++)
            print_long(dir, names[i]);
    }
    else if (horiz_flag)
    {
        print_horizontal(names, count);
    }
    else
    {
        print_columns(names, count);
    }

    // Free everything we allocated
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

void print_long(const char *dir, const char *name)
{
    // lstat needs the full path, not just the file name
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    struct stat st;
    if (lstat(path, &st) == -1)
    {
        perror("lstat");
        return;
    }

    // 1. File type character (from st_mode using macros)
    char mode[11];
    if (S_ISDIR(st.st_mode))       mode[0] = 'd';
    else if (S_ISLNK(st.st_mode))  mode[0] = 'l';
    else if (S_ISCHR(st.st_mode))  mode[0] = 'c';
    else if (S_ISBLK(st.st_mode))  mode[0] = 'b';
    else if (S_ISFIFO(st.st_mode)) mode[0] = 'p';
    else if (S_ISSOCK(st.st_mode)) mode[0] = 's';
    else                           mode[0] = '-';

    // 2. Permissions (bitwise AND with permission masks)
    mode[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    mode[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    mode[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
    mode[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
    mode[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
    mode[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
    mode[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
    mode[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
    mode[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
    mode[10] = '\0';

    // 3. Special bits: setuid, setgid, sticky
    if (st.st_mode & S_ISUID) mode[3] = (mode[3] == 'x') ? 's' : 'S';
    if (st.st_mode & S_ISGID) mode[6] = (mode[6] == 'x') ? 's' : 'S';
    if (st.st_mode & S_ISVTX) mode[9] = (mode[9] == 'x') ? 't' : 'T';

    // 4. Owner and group names (fall back to numeric id if lookup fails)
    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);
    char owner[32], group[32];
    if (pw) snprintf(owner, sizeof(owner), "%s", pw->pw_name);
    else    snprintf(owner, sizeof(owner), "%u", st.st_uid);
    if (gr) snprintf(group, sizeof(group), "%s", gr->gr_name);
    else    snprintf(group, sizeof(group), "%u", st.st_gid);

    // 5. Modification time
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", localtime(&st.st_mtime));

    // 6. Print everything on one line
    printf("%s %2lu %-8s %-8s %8ld %s %s",
           mode,
           (unsigned long)st.st_nlink,
           owner,
           group,
           (long)st.st_size,
           timebuf,
           name);

    // 7. For symlinks, also show the target
    if (S_ISLNK(st.st_mode))
    {
        char target[PATH_MAX];
        ssize_t len = readlink(path, target, sizeof(target) - 1);
        if (len != -1)
        {
            target[len] = '\0';
            printf(" -> %s", target);
        }
    }
    printf("\n");
}

void print_columns(char **names, int count)
{
    if (count == 0)
        return;

    // 1. Terminal width (fall back to 80 if ioctl fails, e.g. output is piped)
    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        term_width = w.ws_col;

    // 2. Longest filename decides the column width
    size_t maxlen = 0;
    for (int i = 0; i < count; i++)
    {
        size_t len = strlen(names[i]);
        if (len > maxlen)
            maxlen = len;
    }
    int col_width = maxlen + 2; // 2 spaces gap between columns

    // 3. How many columns fit, and how many rows that needs
    int cols = term_width / col_width;
    if (cols < 1)
        cols = 1;
    int rows = (count + cols - 1) / cols; // ceiling division

    // 4. "Down then across": item index = column * rows + row
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = c * rows + r;
            if (idx >= count)
                break;
            printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }
}

void print_horizontal(char **names, int count)
{
    if (count == 0)
        return;

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        term_width = w.ws_col;

    size_t maxlen = 0;
    for (int i = 0; i < count; i++)
    {
        size_t len = strlen(names[i]);
        if (len > maxlen)
            maxlen = len;
    }
    int col_width = maxlen + 2;

    // Print left to right; start a new line when the next item won't fit
    int pos = 0;
    for (int i = 0; i < count; i++)
    {
        if (pos > 0 && pos + col_width > term_width)
        {
            printf("\n");
            pos = 0;
        }
        printf("%-*s", col_width, names[i]);
        pos += col_width;
    }
    printf("\n");
}

int compare_names(const void *a, const void *b)
{
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;

    // Case-insensitive first, like the real ls; strcmp breaks ties
    int result = strcasecmp(s1, s2);
    if (result != 0)
        return result;
    return strcmp(s1, s2);
}
