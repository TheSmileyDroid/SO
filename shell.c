#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>


void run(char *input) {
    char *args[100];
    char *token = strtok(input, " ");
    int i = 0;

    while (token != NULL) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    args[i] = NULL;

    pid_t pid = fork();

    if (pid == 0) {
        execvp(args[0], args);
        printf("Command not found: %s \n", args[0]);
    } else {
        wait(NULL);
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

        run(input);
    }

    return 0;
}

