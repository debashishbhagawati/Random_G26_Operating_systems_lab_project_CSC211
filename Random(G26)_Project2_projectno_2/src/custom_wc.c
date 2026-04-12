#define _POSIX_C_SOURCE 200809L
/*
 * custom_wc - Word, line, byte/character count
 * Supports: -l (lines), -w (words), -c (bytes), -m (chars), -L (max line length)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

static int opt_lines  = 0;
static int opt_words  = 0;
static int opt_bytes  = 0;
static int opt_chars  = 0;
static int opt_maxlen = 0;

static void wc_file(FILE *fp, const char *name,
                    long *tl, long *tw, long *tb, long *tmax)
{
    long lines = 0, words = 0, bytes = 0, curlen = 0, maxlen = 0;
    int c, in_word = 0;

    while ((c = fgetc(fp)) != EOF) {
        bytes++;
        if (c == '\n') {
            lines++;
            if (curlen > maxlen) maxlen = curlen;
            curlen = 0;
        } else {
            curlen++;
        }
        if (isspace(c)) {
            in_word = 0;
        } else {
            if (!in_word) { words++; in_word = 1; }
        }
    }
    /* last line without newline */
    if (curlen > maxlen) maxlen = curlen;

    if (tl) *tl += lines;
    if (tw) *tw += words;
    if (tb) *tb += bytes;
    if (tmax && maxlen > *tmax) *tmax = maxlen;

    /* print */
    if (opt_lines)  printf(" %7ld", lines);
    if (opt_words)  printf(" %7ld", words);
    if (opt_bytes || opt_chars) printf(" %7ld", bytes);
    if (opt_maxlen) printf(" %7ld", maxlen);
    if (name)       printf(" %s", name);
    printf("\n");
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "lwcmL")) != -1) {
        switch (opt) {
            case 'l': opt_lines  = 1; break;
            case 'w': opt_words  = 1; break;
            case 'c': opt_bytes  = 1; break;
            case 'm': opt_chars  = 1; break;
            case 'L': opt_maxlen = 1; break;
            default:
                fprintf(stderr, "Usage: custom_wc [-lwcmL] [file...]\n");
                return 1;
        }
    }

    /* default: all three */
    if (!opt_lines && !opt_words && !opt_bytes && !opt_chars && !opt_maxlen)
        opt_lines = opt_words = opt_bytes = 1;

    long tl = 0, tw = 0, tb = 0, tmax = 0;
    int file_count = argc - optind;

    if (file_count == 0) {
        wc_file(stdin, NULL, &tl, &tw, &tb, &tmax);
        return 0;
    }

    for (int i = optind; i < argc; i++) {
        if (strcmp(argv[i], "-") == 0) {
            wc_file(stdin, "-", &tl, &tw, &tb, &tmax);
        } else {
            FILE *fp = fopen(argv[i], "r");
            if (!fp) {
                fprintf(stderr, "custom_wc: %s: %s\n", argv[i], strerror(errno));
                continue;
            }
            wc_file(fp, argv[i], &tl, &tw, &tb, &tmax);
            fclose(fp);
        }
    }

    if (file_count > 1) {
        if (opt_lines)  printf(" %7ld", tl);
        if (opt_words)  printf(" %7ld", tw);
        if (opt_bytes || opt_chars)  printf(" %7ld", tb);
        if (opt_maxlen) printf(" %7ld", tmax);
        printf(" total\n");
    }
    return 0;
}
