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

void process(char *args[100]) {
    int pos = get_pos(args, "|");

    if (pos == -1) {
        run(args);
    } else {
        char *args1[100];
        char *args2[100];

        int i = 0;
        while (i < pos) {
            args1[i] = args[i];
            i++;
        }
        args1[i] = NULL;

        i = 0;
        while (args[pos + 1] != NULL) {
            args2[i] = args[pos + 1];
            pos++;
            i++;
        }
        args2[i] = NULL;

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

        int size = strcspn(input, "\n");
        input[size] = 0;

        if (strcmp(input, "exit") == 0) {
            break;
        }

        char *args[100];
        char *token = strtok(input, " ");
        int i = 0;

        while (token != NULL) {
            args[i] = token;
            token = strtok(NULL, " ");
            i++;
        }

        args[i] = NULL;

        process(args);
        }

    return 0;
}

