#define _POSIX_C_SOURCE 200809L
#if defined(__linux__)
#  define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

static int opt_recursive    = 0;
static int opt_force        = 0;
static int opt_verbose      = 0;
static int opt_interactive  = 0;
static int opt_dir          = 0;

/* safer input handling */
static int ask(const char *prompt) {
    fprintf(stderr, "%s ", prompt);
    fflush(stderr);

    int c = fgetc(stdin);
    if (c == EOF) return 0;

    int ch;
    while ((ch = fgetc(stdin)) != '\n' && ch != EOF);

    return (c == 'y' || c == 'Y');
}

static int rm_path(const char *path);

/* safe path join */
static int build_path(char *dest, size_t size, const char *p1, const char *p2) {
    int n = snprintf(dest, size, "%s/%s", p1, p2);
    return (n < 0 || (size_t)n >= size) ? -1 : 0;
}

static int rm_dir(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        if (!opt_force)
            fprintf(stderr, "custom_rm: cannot open '%s': %s\n", path, strerror(errno));
        return opt_force ? 0 : 1;
    }

    struct dirent *de;
    int ret = 0;

    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        char child[PATH_MAX];

        if (build_path(child, sizeof(child), path, de->d_name) != 0) {
            fprintf(stderr, "custom_rm: path too long: %s/%s\n", path, de->d_name);
            ret = 1;
            continue;
        }

        ret |= rm_path(child);
    }

    closedir(d);

    if (ret == 0) {
        if (opt_interactive) {
            char prompt[PATH_MAX + 64];
            snprintf(prompt, sizeof(prompt),
                     "custom_rm: remove directory '%s'?", path);
            if (!ask(prompt)) return 0;
        }

        if (rmdir(path) < 0) {
            if (!opt_force)
                fprintf(stderr, "custom_rm: cannot remove '%s': %s\n",
                        path, strerror(errno));
            return opt_force ? 0 : 1;
        }

        if (opt_verbose)
            printf("removed directory '%s'\n", path);
    }

    return ret;
}

static int rm_path(const char *path) {
    struct stat st;

    if (lstat(path, &st) < 0) {
        if (opt_force && errno == ENOENT)
            return 0;

        fprintf(stderr, "custom_rm: cannot remove '%s': %s\n",
                path, strerror(errno));
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        if (!opt_recursive && !opt_dir) {
            fprintf(stderr,
                    "custom_rm: cannot remove '%s': Is a directory\n", path);
            return 1;
        }

        if (opt_recursive)
            return rm_dir(path);

        /* -d option */
        if (opt_interactive) {
            char prompt[PATH_MAX + 64];
            snprintf(prompt, sizeof(prompt),
                     "custom_rm: remove directory '%s'?", path);
            if (!ask(prompt)) return 0;
        }

        if (rmdir(path) < 0) {
            fprintf(stderr, "custom_rm: cannot remove '%s': %s\n",
                    path, strerror(errno));
            return 1;
        }

        if (opt_verbose)
            printf("removed directory '%s'\n", path);

        return 0;
    }

    /* files / symlinks */
    if (opt_interactive) {
        char prompt[PATH_MAX + 64];
        snprintf(prompt, sizeof(prompt),
                 "custom_rm: remove '%s'?", path);
        if (!ask(prompt)) return 0;
    }

    if (!opt_force && !(st.st_mode & S_IWUSR)) {
        char prompt[PATH_MAX + 80];
        snprintf(prompt, sizeof(prompt),
                 "custom_rm: remove write-protected file '%s'?", path);
        if (!ask(prompt)) return 0;
    }

    if (unlink(path) < 0) {
        if (opt_force && errno == ENOENT)
            return 0;

        fprintf(stderr, "custom_rm: cannot remove '%s': %s\n",
                path, strerror(errno));
        return 1;
    }

    if (opt_verbose)
        printf("removed '%s'\n", path);

    return 0;
}

int main(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "rRfvid")) != -1) {
        switch (opt) {
            case 'r':
            case 'R':
                opt_recursive = 1;
                break;
            case 'f':
                opt_force = 1;
                opt_interactive = 0;
                break;
            case 'v':
                opt_verbose = 1;
                break;
            case 'i':
                opt_interactive = 1;
                opt_force = 0;
                break;
            case 'd':
                opt_dir = 1;
                break;
            default:
                fprintf(stderr,
                        "Usage: custom_rm [-rRfvid] file...\n");
                return 1;
        }
    }

    if (optind >= argc) {
        if (!opt_force) {
            fprintf(stderr,
                    "custom_rm: missing operand\n"
                    "Usage: custom_rm [-rRfvid] file...\n");
            return 1;
        }
        return 0;
    }

    int ret = 0;
    for (int i = optind; i < argc; i++) {
        ret |= rm_path(argv[i]);
    }

    return ret;
}
