#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>

int get_pos(char *args[100], char *str) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], str) == 0) {
            return i;
        }
        i++;
    }
    return -1;
}

void run(char *args[100]) {
    pid_t pid = fork();

    if (pid == 0) {
        execvp(args[0], args);
        printf("Command not found: %s \n", args[0]);
    } else {
        wait(NULL);
    }
}

enum {
    RUN = 0,
    PIPE = 1,
};

int separate(char *args[100], char *args1[100], char *args2[100]) {
    int pipePos = get_pos(args, "|");

    if (pipePos != -1) {
        int i = 0;
        while (i < pipePos) {
            args1[i] = args[i];
            i++;
        }
        args1[i] = NULL;

        i = 0;
        int j = pipePos;
        while (args[j + 1] != NULL) {
            args2[i] = args[j + 1];
            j++;
            i++;
        }
        args2[i] = NULL;
        return PIPE;
    } else {
        return RUN;
    }
}

void process(char *args[100]) {
    char *args1[100] = {NULL};
    char *args2[100] = {NULL};

    int state = separate(args, args1, args2);

    if (state == RUN) {
        run(args);
    } else if (state == PIPE) {
        int fd[2];
        pipe(fd);

        pid_t p1, p2;
        p1 = fork();

        if (p1 == 0) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            execvp(args1[0], args1);
            printf("Command not found: %s \n", args1[0]);
        } else {
            p2 = fork();
            if (p2 == 0) {
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                execvp(args2[0], args2);
                printf("Command not found: %s \n", args2[0]);
            } else {
                close(fd[0]);
                close(fd[1]);
                wait(NULL);
                wait(NULL);
            }
        }
    }
}


int main() {
    while(true) {
        printf("> ");

        char input[100];
        fgets(input, 100, stdin);

        int len = strlen(input);
        if (input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        if (strcmp(input, "exit") == 0) {
            break;
        }

        char *args[100];
        int argc = 0;
        bool in_quotes = false;
        char *arg_start = input;

        for (char *p = input; *p; p++) {
            if (*p == '"' || *p == '\'') {
                in_quotes = !in_quotes;
            } else if (*p == ' ' && !in_quotes) {
                *p = '\0';
                args[argc++] = arg_start;
                arg_start = p + 1;
            }
        }

        args[argc++] = arg_start;
        args[argc] = NULL;

        for (int i = 0; i < argc; i++) {
            char *arg = args[i];
            int len = strlen(arg);
            if (arg[0] == '"' && arg[len - 1] == '"') {
                arg[len - 1] = '\0';
                args[i] = arg + 1;
            } else if (arg[0] == '\'' && arg[len - 1] == '\'') {
                arg[len - 1] = '\0';
                args[i] = arg + 1;
            }
        }

        process(args);
        } 

    return 0;
}

