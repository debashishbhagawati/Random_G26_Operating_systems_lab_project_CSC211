/*
 * custom_grep - Search for patterns in files
 * Supports: -i (ignore case), -v (invert match), -n (line numbers),
 *           -c (count), -l (list files), -r (recursive), -e (pattern),
 *           -w (whole word), -x (whole line), -q (quiet), -H (print filename),
 *           -h (suppress filename)
 */
#if defined(__linux__)
#  define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static int opt_ignore_case  = 0;
static int opt_invert       = 0;
static int opt_line_num     = 0;
static int opt_count        = 0;
static int opt_list_files   = 0;
static int opt_recursive    = 0;
static int opt_whole_word   = 0;
static int opt_whole_line   = 0;
static int opt_quiet        = 0;
static int opt_force_fname  = 0;  /* -H */
static int opt_no_fname     = 0;  /* -h */
static int multi_file       = 0;
static int found_any        = 0;

static char *pattern_str    = NULL;

static int do_grep(const char *filename, FILE *fp, regex_t *re) {
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    long lineno = 0;
    long match_count = 0;
    int print_fname = (multi_file || opt_force_fname) && !opt_no_fname;

    while ((len = getline(&line, &cap, fp)) != -1) {
        lineno++;
        /* strip trailing newline for matching */
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

        int matched = (regexec(re, line, 0, NULL, 0) == 0);
        if (opt_invert) matched = !matched;

        if (matched) {
            found_any = 1;
            match_count++;
            if (opt_quiet) { free(line); return 1; }
            if (opt_list_files) {
                printf("%s\n", filename ? filename : "(stdin)");
                free(line);
                return 1;
            }
            if (!opt_count) {
                if (print_fname)  printf("%s:", filename);
                if (opt_line_num) printf("%ld:", lineno);
                printf("%s\n", line);
            }
        }
    }
    free(line);

    if (opt_count) {
        if (print_fname) printf("%s:", filename ? filename : "(stdin)");
        printf("%ld\n", match_count);
    }
    return match_count > 0;
}

static void grep_path(const char *path, regex_t *re);

static void grep_dir(const char *dirpath, regex_t *re) {
    DIR *d = opendir(dirpath);
    if (!d) { perror(dirpath); return; }
    struct dirent *de;
    while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        char child[4096];
        snprintf(child, sizeof(child), "%s/%s", dirpath, de->d_name);
        grep_path(child, re);
    }
    closedir(d);
}

static void grep_path(const char *path, regex_t *re) {
    struct stat st;
    if (stat(path, &st) < 0) { perror(path); return; }
    if (S_ISDIR(st.st_mode)) {
        if (opt_recursive) grep_dir(path, re);
        else fprintf(stderr, "custom_grep: %s: Is a directory\n", path);
        return;
    }
    FILE *fp = fopen(path, "r");
    if (!fp) { fprintf(stderr, "custom_grep: %s: %s\n", path, strerror(errno)); return; }
    do_grep(path, fp, re);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "ivncle:rwxqHhr")) != -1) {
        switch (opt) {
            case 'i': opt_ignore_case = 1; break;
            case 'v': opt_invert      = 1; break;
            case 'n': opt_line_num    = 1; break;
            case 'c': opt_count       = 1; break;
            case 'l': opt_list_files  = 1; break;
            case 'r': opt_recursive   = 1; break;
            case 'w': opt_whole_word  = 1; break;
            case 'x': opt_whole_line  = 1; break;
            case 'q': opt_quiet       = 1; break;
            case 'H': opt_force_fname = 1; break;
            case 'h': opt_no_fname    = 1; break;
            case 'e': pattern_str     = optarg; break;
            default:
                fprintf(stderr, "Usage: custom_grep [-ivnclerwxqHh] [-e pattern] pattern [file...]\n");
                return 2;
        }
    }

    if (!pattern_str) {
        if (optind >= argc) {
            fprintf(stderr, "custom_grep: missing pattern\n");
            return 2;
        }
        pattern_str = argv[optind++];
    }

    /* Build regex pattern */
    char pat[4096];
    if (opt_whole_line)
        snprintf(pat, sizeof(pat), "^(%s)$", pattern_str);
    else if (opt_whole_word)
        snprintf(pat, sizeof(pat), "(^|[^[:alnum:]_])(%s)([^[:alnum:]_]|$)", pattern_str);
    else
        snprintf(pat, sizeof(pat), "%s", pattern_str);

    int flags = REG_EXTENDED | (opt_ignore_case ? REG_ICASE : 0);
    regex_t re;
    if (regcomp(&re, pat, flags) != 0) {
        fprintf(stderr, "custom_grep: invalid regex: %s\n", pattern_str);
        return 2;
    }

    multi_file = (argc - optind > 1);

    if (optind >= argc) {
        do_grep(NULL, stdin, &re);
    } else {
        for (int i = optind; i < argc; i++)
            grep_path(argv[i], &re);
    }

    regfree(&re);
    return found_any ? 0 : 1;
}
