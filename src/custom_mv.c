#define _POSIX_C_SOURCE 200809L
/*
 * custom_mv - Move or rename files
 * Supports: -v (verbose), -f (force), -n (no clobber), -u (update),
 *           -i (interactive)
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

#define BUF_SIZE (1024 * 1024)

static int opt_verbose     = 0;
static int opt_force       = 0;
static int opt_noclobber   = 0;
static int opt_update      = 0;
static int opt_interactive = 0;

/* Cross-device copy+delete fallback */
static int copy_file_raw(const char *src, const char *dst, struct stat *src_st) {
    int in_fd = open(src, O_RDONLY);
    if (in_fd < 0) { perror(src); return 1; }

    int out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, src_st->st_mode & 0777);
    if (out_fd < 0) { perror(dst); close(in_fd); return 1; }

    char *buf = malloc(BUF_SIZE);
    if (!buf) { perror("malloc"); close(in_fd); close(out_fd); return 1; }

    ssize_t nr, nw;
    int ret = 0;
    while ((nr = read(in_fd, buf, BUF_SIZE)) > 0) {
        char *p = buf;
        while (nr > 0) {
            nw = write(out_fd, p, nr);
            if (nw < 0) { perror("write"); ret = 1; goto done; }
            p += nw; nr -= nw;
        }
    }
    if (nr < 0) { perror("read"); ret = 1; }
done:
    free(buf);
    close(in_fd);
    close(out_fd);
    if (ret == 0) {
        struct utimbuf ut = { src_st->st_atime, src_st->st_mtime };
        utime(dst, &ut);
        chmod(dst, src_st->st_mode);
        unlink(src);
    }
    return ret;
}

static int copy_dir_recursive(const char *src, const char *dst);

static int move_path(const char *src, const char *dst_path);

static int copy_dir_recursive(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) < 0) { perror(src); return 1; }
    if (mkdir(dst, st.st_mode) < 0 && errno != EEXIST) { perror(dst); return 1; }

    DIR *d = opendir(src);
    if (!d) { perror(src); return 1; }
    struct dirent *de;
    int ret = 0;
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name,".") || !strcmp(de->d_name,"..")) continue;
        char s[4096], t[4096];
        snprintf(s, sizeof(s), "%s/%s", src, de->d_name);
        snprintf(t, sizeof(t), "%s/%s", dst, de->d_name);
        ret |= move_path(s, t);
    }
    closedir(d);
    rmdir(src);
    return ret;
}

static int move_path(const char *src, const char *dst) {
    struct stat src_st, dst_st;
    if (lstat(src, &src_st) < 0) {
        fprintf(stderr, "custom_mv: cannot stat '%s': %s\n", src, strerror(errno));
        return 1;
    }

    int dst_exists = (lstat(dst, &dst_st) == 0);

    if (opt_noclobber && dst_exists) return 0;

    if (opt_update && dst_exists && dst_st.st_mtime >= src_st.st_mtime) return 0;

    if (opt_interactive && dst_exists) {
        fprintf(stderr, "custom_mv: overwrite '%s'? ", dst);
        int c = fgetc(stdin);
        int flush; while ((flush = fgetc(stdin)) != '\n' && flush != EOF);
        if (c != 'y' && c != 'Y') return 0;
    }

    if (!opt_force && dst_exists && !(dst_st.st_mode & S_IWUSR)) {
        fprintf(stderr, "custom_mv: cannot overwrite '%s': Permission denied\n", dst);
        return 1;
    }

    /* Try atomic rename first */
    if (rename(src, dst) == 0) {
        if (opt_verbose) printf("'%s' -> '%s'\n", src, dst);
        return 0;
    }

    if (errno == EXDEV) {
        /* cross-device: copy then remove */
        int ret;
        if (S_ISDIR(src_st.st_mode))
            ret = copy_dir_recursive(src, dst);
        else
            ret = copy_file_raw(src, dst, &src_st);
        if (ret == 0 && opt_verbose)
            printf("'%s' -> '%s'\n", src, dst);
        return ret;
    }

    fprintf(stderr, "custom_mv: cannot move '%s' to '%s': %s\n", src, dst, strerror(errno));
    return 1;
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "vfniu")) != -1) {
        switch (opt) {
            case 'v': opt_verbose     = 1; break;
            case 'f': opt_force       = 1; opt_interactive = 0; break;
            case 'n': opt_noclobber   = 1; break;
            case 'i': opt_interactive = 1; opt_force       = 0; break;
            case 'u': opt_update      = 1; break;
            default:
                fprintf(stderr, "Usage: custom_mv [-vfniu] source... dest\n");
                return 1;
        }
    }

    int remaining = argc - optind;
    if (remaining < 2) {
        fprintf(stderr, "custom_mv: missing operand\nUsage: custom_mv [-vfniu] source... dest\n");
        return 1;
    }

    const char *dst = argv[argc - 1];
    struct stat dst_st;
    int dst_is_dir = (stat(dst, &dst_st) == 0 && S_ISDIR(dst_st.st_mode));

    if (remaining > 2 && !dst_is_dir) {
        fprintf(stderr, "custom_mv: target '%s' is not a directory\n", dst);
        return 1;
    }

    int ret = 0;
    for (int i = optind; i < argc - 1; i++) {
        char target[4096];
        if (dst_is_dir) {
            const char *base = strrchr(argv[i], '/');
            base = base ? base + 1 : argv[i];
            snprintf(target, sizeof(target), "%s/%s", dst, base);
        } else {
            snprintf(target, sizeof(target), "%s", dst);
        }
        ret |= move_path(argv[i], target);
    }
    return ret;
}
