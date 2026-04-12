#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "1.0.0"


void print_usage(void) {
    printf("Usage: custom_echo [OPTION]... [STRING]...\n");
    printf("Echo the STRING(s) to standard output.\n\n");
    printf("  -n          do not output the trailing newline\n");
    printf("  -e          enable interpretation of backslash escapes\n");
    printf("  -E          disable interpretation of backslash escapes (default)\n");
    printf("  --help      display this help and exit\n");
    printf("  --version   output version information and exit\n\n");
    printf("Supported escape sequences with -e:\n");
    printf("  \\\\   backslash\n");
    printf("  \\n   newline\n");
    printf("  \\t   horizontal tab\n");
    printf("  \\r   carriage return\n");
    printf("  \\a   alert / bell\n");
    printf("  \\b   backspace\n");
    printf("  \\0N  octal value (up to 3 digits)\n");
}

void print_version(void) {
    printf("custom_echo version %s\n", VERSION);
    printf("A simplified UNIX-like echo utility written in C.\n");
}


char *process_escapes(const char *src) {
    size_t len=strlen(src);
    
    char  *buf=malloc(len+1);
    if (!buf) {
        perror("custom_echo: malloc");
        exit(EXIT_FAILURE);
    }

    size_t i=0,j=0;
    while (i<len) {
        if (src[i]=='\\' && i+1<len) {
            i++;                        
            switch (src[i]) {
                case '\\': buf[j++] = '\\'; break;
                case 'n':  buf[j++] = '\n'; break;
                case 't':  buf[j++] = '\t'; break;
                case 'r':  buf[j++] = '\r'; break;
                case 'a':  buf[j++] = '\a'; break;
                case 'b':  buf[j++] = '\b'; break;
                case '0': {
                    
                    int octal=0,k;
                    for (k =0;k<3 && i+1<len &&
                                src[i+1]>='0' && src[i+1]<='7'; k++) {
                        i++;
                        octal=octal*8+(src[i]-'0');
                    }
                    buf[j++]=(char)octal;
                    break;
                }
                default:
                    
                    buf[j++]='\\';
                    buf[j++]=src[i];
                    break;
            }
        } else {
            buf[j++]=src[i];
        }
        i++;
    }
    buf[j]='\0';
    return buf;
}


int main(int argc, char *argv[]) {
    int no_newline=0;   // -n flag 
    int escape_enabled=0;   // -e flag 
    int first_string=1;   // index of first non-option argument 

    
    int i;
    for (i=1;i<argc;i++) {
        
        if (strcmp(argv[i],"--help") == 0) {
            print_usage();
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--version") == 0) {
            print_version();
            return EXIT_SUCCESS;
        }

        
        if (argv[i][0]=='-' && argv[i][1]!='\0') {
            int valid_flag = 1;
            int k;
            for (k = 1; argv[i][k] != '\0'; k++) {
                if (argv[i][k] =='n') {
                    no_newline = 1;
                } else if (argv[i][k]=='e') {
                    escape_enabled = 1;
                } else if (argv[i][k]=='E') {
                    escape_enabled=0;
                } else {
                    valid_flag=0;
                    break;
                }
            }
            if (valid_flag) {
                first_string=i+1;   
                continue;
            }
        }

        
        first_string=i;
        break;
    }

    
    for (i = first_string; i<argc; i++) {
        if (i > first_string) {
            putchar(' ');   
        }

        if (escape_enabled) {
            char *processed = process_escapes(argv[i]);
            fputs(processed, stdout);
            free(processed);
        } else {
            fputs(argv[i], stdout);
        }
    }

    if (!no_newline) {
        putchar('\n');
    }

    return EXIT_SUCCESS;
}
