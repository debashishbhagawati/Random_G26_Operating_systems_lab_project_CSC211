#define _POSIX_C_SOURCE 200809L
/*
 * custom_cp - Copy files or directories
 * Supports: -r/-R (recursive), -v (verbose), -f (force), -p (preserve attrs),
 *           -n (no clobber), -u (update only if newer)
 */
#if defined(__linux__)
#  define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <utime.h>

#define BUF_SIZE (1024 * 1024)  /* 1 MiB copy buffer */

static int opt_recursive = 0;
static int opt_verbose   = 0;
static int opt_force     = 0;
static int opt_preserve  = 0;
static int opt_noclobber = 0;
static int opt_update    = 0;

static int copy_file(const char *src, const char *dst);
static int copy_path(const char *src, const char *dst);

static int copy_file(const char *src, const char *dst) {
    struct stat src_st, dst_st;
    if (stat(src, &src_st) < 0) {
        fprintf(stderr, "custom_cp: cannot stat '%s': %s\n", src, strerror(errno));
        return 1;
    }

    /* -n: don't overwrite */
    if (opt_noclobber && stat(dst, &dst_st) == 0) return 0;

    /* -u: skip if dst is same age or newer */
    if (opt_update && stat(dst, &dst_st) == 0) {
        if (dst_st.st_mtime >= src_st.st_mtime) return 0;
    }

    int in_fd = open(src, O_RDONLY);
    if (in_fd < 0) {
        fprintf(stderr, "custom_cp: cannot open '%s': %s\n", src, strerror(errno));
        return 1;
    }

    int flags = O_WRONLY | O_CREAT | (opt_force ? O_TRUNC : O_TRUNC);
    int out_fd = open(dst, flags, src_st.st_mode & 0777);
    if (out_fd < 0) {
        if (opt_force) { unlink(dst); out_fd = open(dst, flags, src_st.st_mode & 0777); }
        if (out_fd < 0) {
            fprintf(stderr, "custom_cp: cannot create '%s': %s\n", dst, strerror(errno));
            close(in_fd); return 1;
        }
    }

    char *buf = malloc(BUF_SIZE);
    if (!buf) { perror("malloc"); close(in_fd); close(out_fd); return 1; }

    ssize_t nr, nw;
    int ret = 0;
    while ((nr = read(in_fd, buf, BUF_SIZE)) > 0) {
        char *p = buf;
        while (nr > 0) {
            nw = write(out_fd, p, nr);
            if (nw < 0) {
                fprintf(stderr, "custom_cp: write error '%s': %s\n", dst, strerror(errno));
                ret = 1; goto done;
            }
            p += nw; nr -= nw;
        }
    }
    if (nr < 0) {
        fprintf(stderr, "custom_cp: read error '%s': %s\n", src, strerror(errno));
        ret = 1;
    }

done:
    free(buf);
    close(in_fd);
    close(out_fd);

    if (ret == 0 && opt_preserve) {
        struct utimbuf ut = { src_st.st_atime, src_st.st_mtime };
        utime(dst, &ut);
        chmod(dst, src_st.st_mode);
    }

    if (ret == 0 && opt_verbose)
        printf("'%s' -> '%s'\n", src, dst);

    return ret;
}

static int copy_dir(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) < 0) { perror(src); return 1; }

    if (mkdir(dst, st.st_mode) < 0 && errno != EEXIST) {
        fprintf(stderr, "custom_cp: cannot mkdir '%s': %s\n", dst, strerror(errno));
        return 1;
    }

    DIR *d = opendir(src);
    if (!d) { perror(src); return 1; }

    struct dirent *de;
    int ret = 0;
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char s[4096], t[4096];
        snprintf(s, sizeof(s), "%s/%s", src, de->d_name);
        snprintf(t, sizeof(t), "%s/%s", dst, de->d_name);
        ret |= copy_path(s, t);
    }
    closedir(d);
    return ret;
}

static int copy_path(const char *src, const char *dst) {
    struct stat st;
    if (lstat(src, &st) < 0) { perror(src); return 1; }
    if (S_ISDIR(st.st_mode)) {
        if (!opt_recursive) {
            fprintf(stderr, "custom_cp: -r not specified; omitting directory '%s'\n", src);
            return 1;
        }
        return copy_dir(src, dst);
    }
    return copy_file(src, dst);
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "rRvfpnu")) != -1) {
        switch (opt) {
            case 'r': case 'R': opt_recursive = 1; break;
            case 'v': opt_verbose   = 1; break;
            case 'f': opt_force     = 1; break;
            case 'p': opt_preserve  = 1; break;
            case 'n': opt_noclobber = 1; break;
            case 'u': opt_update    = 1; break;
            default:
                fprintf(stderr, "Usage: custom_cp [-rRvfpnu] source... dest\n");
                return 1;
        }
    }

    int remaining = argc - optind;
    if (remaining < 2) {
        fprintf(stderr, "custom_cp: missing operand\nUsage: custom_cp [-rRvfpnu] source... dest\n");
        return 1;
    }

    const char *dst = argv[argc - 1];
    struct stat dst_st;
    int dst_is_dir = (stat(dst, &dst_st) == 0 && S_ISDIR(dst_st.st_mode));

    if (remaining > 2 && !dst_is_dir) {
        fprintf(stderr, "custom_cp: target '%s' is not a directory\n", dst);
        return 1;
    }

    int ret = 0;
    for (int i = optind; i < argc - 1; i++) {
        char target[4096];
        if (dst_is_dir) {
            /* find basename of source */
            const char *base = strrchr(argv[i], '/');
            base = base ? base + 1 : argv[i];
            snprintf(target, sizeof(target), "%s/%s", dst, base);
        } else {
            snprintf(target, sizeof(target), "%s", dst);
        }
        ret |= copy_path(argv[i], target);
    }
    return ret;
}
