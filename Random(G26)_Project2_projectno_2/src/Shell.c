#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_INPUT  1024
#define MAX_ARGS    128
#define PROMPT     "myshell> "

static int parse_input(char *line, char **args) {
    int count=0;
    char *token=strtok(line," \t");
    while(token && count<MAX_ARGS-1){
        args[count++]=token;
        token=strtok(NULL," \t");
    }
    args[count]=NULL;
    return count;
}

static void run_custom_wc(char **args) {
    char local_path[512];
    snprintf(local_path, sizeof(local_path), "./custom_wc");

    pid_t pid=fork();
    if (pid<0){
        perror("fork");
        return;
    }
    if (pid==0) {
        args[0]=local_path;
        execv(local_path,args);
        fprintf(stderr,"myshell: custom_wc: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    int status;
    waitpid(pid, &status, 0);
}

static void run_custom_echo(char **args) {
    char local_path[512];
    snprintf(local_path, sizeof(local_path), "./custom_echo");

    pid_t pid=fork();
    if (pid<0){
        perror("fork");
        return;
    }
    if (pid==0) {
        args[0]=local_path;
        execv(local_path,args);
        fprintf(stderr,"myshell: custom_echo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    int status;
    waitpid(pid, &status, 0);
}


static void run_custom_cat(char **args) {
    char local_path[512];
    snprintf(local_path, sizeof(local_path), "./custom_cat");

    pid_t pid=fork();
    if (pid<0){
        perror("fork");
        return;
    }
    if (pid==0) {
        args[0]=local_path;
        execv(local_path,args);
        fprintf(stderr,"myshell: custom_cat: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    int status;
    waitpid(pid, &status, 0);
}


static void run_custom_ls(char **args) {
    char local_path[512];
    snprintf(local_path, sizeof(local_path), "./custom_ls");

    pid_t pid=fork();
    if (pid<0){
        perror("fork");
        return;
    }
    if (pid==0) {
        args[0]=local_path;
        execv(local_path,args);
        fprintf(stderr,"myshell: custom_ls: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    int status;
    waitpid(pid, &status, 0);
}

int main(void) {
    char  line[MAX_INPUT];
    char *args[MAX_ARGS];

    printf("=== Custom OS Shell (type 'exit' to quit) ===\n\n");

    while (1) {
        printf("%s",PROMPT);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        line[strcspn(line,"\n")] = '\0';
        if (line[0]=='\0') continue;

        int count = parse_input(line,args);
        if (count==0) continue;

        if (strcmp(args[0],"exit")==0) {
            printf("Goodbye!\n");
            break;
        }

        // Updated command routing logic
        if (strcmp(args[0],"custom_echo") == 0) {
            run_custom_echo(args);
        } else if(strcmp(args[0],"custom_wc") == 0) {
            run_custom_wc(args);
        } else if(strcmp(args[0],"custom_cat") == 0) {
            run_custom_cat(args);
        } else if(strcmp(args[0],"custom_ls") == 0) {
            run_custom_ls(args);
        } else {
            fprintf(stderr, "myshell: unknown command '%s'.\n", args[0]);
        }
    }

    return EXIT_SUCCESS;
}
