#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

extern int errno;

void do_ls(const char *dir, int long_flag);
void print_long(const char *dir, const char *name);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;

    // getopt scans argv for options like -l
    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [dir...]\n", argv[0]);
            exit(1);
        }
    }

    // optind = index of first non-option argument (a directory name)
    if (optind == argc)
    {
        do_ls(".", long_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s :\n", argv[i]);
            do_ls(argv[i], long_flag);
            puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir, int long_flag)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_flag)
            print_long(dir, entry->d_name);
        else
            printf("%s\n", entry->d_name);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

void print_long(const char *dir, const char *name)
{
    // stat needs the full path, not just the file name
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

    // For symlinks, also show the target
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
