#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    char *name;
    int argc;
    char *args[100];
} Command;

int parseCommand(char *input, Command *command) {
    char *token = strtok(input, " ");
    command->name = token;
    int i = 0;
    while(token != NULL) {
        command->args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    command->argc = i;
    command->args[i] = NULL;
    return 0;
}

Command *newCommand(char *strargs) {
    Command *command = malloc(sizeof(Command));
    parseCommand(strargs, command);

    return command;
}

void logCommand(Command *command) {
    FILE *logFile = fopen("log.txt", "a");
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char time[100];
    strftime(time, 100, "%d/%m/%Y %H:%M:%S", tm);
    fprintf(logFile, "[%s] Comando sendo executado: %s, args: [", time, command->name);
    for(int i = 0; i < command->argc; i++) {
        fprintf(logFile, "%s", command->args[i]);
        if (i != command->argc - 1) {
            fprintf(logFile, ", ");
        }
    }
    fprintf(logFile, "]\n");
    fclose(logFile);
}

int processCommand(Command *command) {
    return execvp(command->name, command->args);
}


int main() {
    while(true) {
        printf("> ");

        char input[100];
        fgets(input, 100, stdin);

        int size = strcspn(input, "\n");
        input[size] = 0;

        Command *command = newCommand(input);

        logCommand(command);

        pid_t pid = fork();

        if(pid == 0) { // Processo filho
            if (processCommand(command) == -1) {
                printf("Comando não encontrado\n");
            }
            exit(0);
        } else {
            wait(NULL);
        }
    }

    return 0;
}