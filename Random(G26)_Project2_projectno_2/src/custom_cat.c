/*
 * custom_cat - Concatenate and display file content
 * Supports: -n (number lines), -b (number non-blank), -A (show all),
 *           -v (show non-printing), -E (show $ at EOL), -T (show tabs as ^I),
 *           -s (squeeze blank lines)
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

static int opt_number       = 0;
static int opt_number_nb    = 0;
static int opt_show_ends    = 0;
static int opt_show_tabs    = 0;
static int opt_show_nonprint= 0;
static int opt_squeeze      = 0;

static void cat_file(FILE *fp) {
    static long lineno = 0;
    int c, prev = '\n';
    int blank_count = 0;
    int line_started = 0;
    int num_printed = 0;

    while ((c = fgetc(fp)) != EOF) {
        /* squeeze blank lines */
        if (opt_squeeze) {
            if (c == '\n' && prev == '\n') {
                blank_count++;
                if (blank_count > 1) { prev = c; continue; }
            } else {
                blank_count = 0;
            }
        }

        /* line numbering at start of line */
        if (prev == '\n') {
            line_started = 1;
            num_printed = 0;
            if (opt_number && !opt_number_nb) {
                printf("%6ld\t", ++lineno);
                num_printed = 1;
            }
            /* for -b (number non-blank), wait to see non-whitespace */
        }

        /* for -b, print number on first non-whitespace char of line */
        if (opt_number_nb && !num_printed && line_started && !isspace(c) && c != '\n') {
            printf("%6ld\t", ++lineno);
            num_printed = 1;
        }

        if (c == '\n') {
            if (opt_show_ends) putchar('$');
            putchar('\n');
            line_started = 0;
        } else if (c == '\t' && opt_show_tabs) {
            printf("^I");
        } else if (opt_show_nonprint && (c < 32 || c == 127) && c != '\t') {
            if (c < 32)       printf("^%c", c + 64);
            else              printf("^?");
        } else {
            putchar(c);
        }
        prev = c;
    }
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "nbAETvs")) != -1) {
        switch (opt) {
            case 'n': opt_number        = 1; break;
            case 'b': opt_number_nb     = 1; opt_number = 0; break;
            case 'A': opt_show_ends     = 1; opt_show_tabs = 1; opt_show_nonprint = 1; break;
            case 'E': opt_show_ends     = 1; break;
            case 'T': opt_show_tabs     = 1; break;
            case 'v': opt_show_nonprint = 1; break;
            case 's': opt_squeeze       = 1; break;
            default:
                fprintf(stderr, "Usage: custom_cat [-nbAETvs] [file...]\n");
                return 1;
        }
    }

    if (optind >= argc) {
        cat_file(stdin);
    } else {
        for (int i = optind; i < argc; i++) {
            if (strcmp(argv[i], "-") == 0) {
                cat_file(stdin);
            } else {
                FILE *fp = fopen(argv[i], "r");
                if (!fp) {
                    fprintf(stderr, "custom_cat: %s: %s\n", argv[i], strerror(errno));
                    continue;
                }
                cat_file(fp);
                fclose(fp);
            }
        }
    }
    return 0;
}
