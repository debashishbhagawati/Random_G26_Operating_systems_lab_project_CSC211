#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 10000
#define MAX_LEN 1024

int main(int argc, char *argv[]) {
    FILE *fp;
    char *filename = NULL;
    int n = 10;
    int show_head = 1, show_tail = 1;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            n = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            show_tail = 0;
        } else if (strcmp(argv[i], "-t") == 0) {
            show_head = 0;
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        fprintf(stderr, "Usage: custom_headtail [OPTIONS] file\n");
        return 1;
    }

    fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    char *lines[MAX_LINES];
    int count = 0;
    char buffer[MAX_LEN];

    // Read file
    while (fgets(buffer, sizeof(buffer), fp)) {
        lines[count] = strdup(buffer);
        count++;
    }

    fclose(fp);

    // HEAD
    if (show_head) {
        printf("---- HEAD (%d lines) ----\n", n);
        for (int i = 0; i < n && i < count; i++) {
            printf("%s", lines[i]);
        }
    }

    // TAIL
    if (show_tail) {
        printf("---- TAIL (%d lines) ----\n", n);
        for (int i = count - n; i < count; i++) {
            if (i >= 0)
                printf("%s", lines[i]);
        }
    }

    // Free memory
    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }

    return 0;
}
