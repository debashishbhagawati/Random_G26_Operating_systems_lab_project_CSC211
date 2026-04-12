/*
 * custom_ls - List directory contents
 * Supports: -l (long format), -a (all files), -h (human-readable sizes),
 *           -r (reverse), -t (sort by time), -1 (one per line)
 */

#if defined(__linux__)
#  define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>   /* NAME_MAX on macOS */
#include <pwd.h>
#include <grp.h>
#include <time.h>
#ifndef NAME_MAX
#  define NAME_MAX 255
#endif
#include <unistd.h>
#include <errno.h>

#define MAX_ENTRIES 8192

typedef struct {
    char name[NAME_MAX + 1];
    struct stat st;
} Entry;

static int opt_long   = 0;
static int opt_all    = 0;
static int opt_human  = 0;
static int opt_reverse= 0;
static int opt_time   = 0;
static int opt_one    = 0;

static void human_size(off_t size, char *buf, size_t buflen) {
    const char *units[] = {"B","K","M","G","T","P"};
    double s = (double)size;
    int u = 0;
    while (s >= 1024.0 && u < 5) { s /= 1024.0; u++; }
    if (u == 0) snprintf(buf, buflen, "%lld", (long long)size);
    else snprintf(buf, buflen, "%.1f%s", s, units[u]);
}

static void print_permissions(mode_t mode) {
    char perm[11];
    perm[0] = S_ISDIR(mode)  ? 'd' :
              S_ISLNK(mode)  ? 'l' :
              S_ISCHR(mode)  ? 'c' :
              S_ISBLK(mode)  ? 'b' :
              S_ISFIFO(mode) ? 'p' :
              S_ISSOCK(mode) ? 's' : '-';
    perm[1] = (mode & S_IRUSR) ? 'r' : '-';
    perm[2] = (mode & S_IWUSR) ? 'w' : '-';
    perm[3] = (mode & S_ISUID) ? 's' : (mode & S_IXUSR) ? 'x' : '-';
    perm[4] = (mode & S_IRGRP) ? 'r' : '-';
    perm[5] = (mode & S_IWGRP) ? 'w' : '-';
    perm[6] = (mode & S_ISGID) ? 's' : (mode & S_IXGRP) ? 'x' : '-';
    perm[7] = (mode & S_IROTH) ? 'r' : '-';
    perm[8] = (mode & S_IWOTH) ? 'w' : '-';
    perm[9] = (mode & S_ISVTX) ? 't' : (mode & S_IXOTH) ? 'x' : '-';
    perm[10] = '\0';
    printf("%s", perm);
}

static int cmp_name(const void *a, const void *b) {
    const Entry *ea = (const Entry *)a;
    const Entry *eb = (const Entry *)b;
    int r = strcasecmp(ea->name, eb->name);
    return opt_reverse ? -r : r;
}

static int cmp_time(const void *a, const void *b) {
    const Entry *ea = (const Entry *)a;
    const Entry *eb = (const Entry *)b;
    if (ea->st.st_mtime < eb->st.st_mtime) return opt_reverse ? -1 : 1;
    if (ea->st.st_mtime > eb->st.st_mtime) return opt_reverse ? 1 : -1;
    return 0;
}

static void list_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) { perror(path); return; }

    Entry *entries = malloc(MAX_ENTRIES * sizeof(Entry));
    if (!entries) { perror("malloc"); closedir(dir); return; }
    int count = 0;

    struct dirent *de;
    while ((de = readdir(dir)) != NULL && count < MAX_ENTRIES) {
        if (!opt_all && de->d_name[0] == '.') continue;
        strncpy(entries[count].name, de->d_name, NAME_MAX);
        entries[count].name[NAME_MAX] = '\0';

        char fullpath[4096];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, de->d_name);
        if (lstat(fullpath, &entries[count].st) < 0) {
            memset(&entries[count].st, 0, sizeof(struct stat));
        }
        count++;
    }
    closedir(dir);

    qsort(entries, count, sizeof(Entry), opt_time ? cmp_time : cmp_name);

    if (opt_long) {
        blkcnt_t total = 0;
        for (int i = 0; i < count; i++) total += entries[i].st.st_blocks;
        printf("total %lld\n", (long long)(total / 2));
    }

    for (int i = 0; i < count; i++) {
        if (opt_long) {
            char timebuf[64];
            struct tm *tm = localtime(&entries[i].st.st_mtime);
            time_t now = time(NULL);
            if (now - entries[i].st.st_mtime < 6*30*24*3600)
                strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm);
            else
                strftime(timebuf, sizeof(timebuf), "%b %e  %Y", tm);

            struct passwd *pw = getpwuid(entries[i].st.st_uid);
            struct group  *gr = getgrgid(entries[i].st.st_gid);
            char owner[32], group[32], sizebuf[32];
            snprintf(owner, sizeof(owner), "%s", pw ? pw->pw_name : "?");
            snprintf(group, sizeof(group), "%s", gr ? gr->gr_name : "?");

            if (opt_human)
                human_size(entries[i].st.st_size, sizebuf, sizeof(sizebuf));
            else
                snprintf(sizebuf, sizeof(sizebuf), "%lld", (long long)entries[i].st.st_size);

            print_permissions(entries[i].st.st_mode);
            printf(" %2lu %-8s %-8s %8s %s %s\n",
                   (unsigned long)entries[i].st.st_nlink,
                   owner, group, sizebuf, timebuf, entries[i].name);
        } else {
            printf("%s", entries[i].name);
            if (opt_one || isatty(STDOUT_FILENO) == 0)
                printf("\n");
            else
                printf("  ");
        }
    }
    if (!opt_long && !opt_one && isatty(STDOUT_FILENO)) printf("\n");
    free(entries);
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "lahrt1")) != -1) {
        switch (opt) {
            case 'l': opt_long    = 1; break;
            case 'a': opt_all     = 1; break;
            case 'h': opt_human   = 1; break;
            case 'r': opt_reverse = 1; break;
            case 't': opt_time    = 1; break;
            case '1': opt_one     = 1; break;
            default:
                fprintf(stderr, "Usage: custom_ls [-lahrt1] [path...]\n");
                return 1;
        }
    }

    if (optind >= argc) {
        list_dir(".")
    } else {
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1) printf("%s:\n", argv[i]);
            list_dir(argv[i]);
            if (argc - optind > 1 && i < argc - 1) printf("\n");
        }
    }
    return 0;
}
