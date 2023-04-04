#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    char *name[20];
    int argc;
    char *args[100];
} Command;

int parseCommand(char *input, char *args[], int *argc) {
    char *token = strtok(input, " ");
    int i = 0;
    while(token != NULL) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    *argc = i;
    args[i] = NULL;
    return 0;
}

Command *newCommand(char *strargs) {
    Command *command = malloc(sizeof(Command));
    parseCommand(strargs, command->args, &command->argc);
    *command->name = command->args[0];

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
    char *arg[100];
    arg[0] = command->args[0];
    return execvp(command->args[0], arg);
}

int processOperator(char *input) {
    char *pipeToken = strstr(input, "|");
    if (pipeToken != NULL) {
        char *cmd1 = strtok(input, "|");
        char *cmd2 = strtok(NULL, "|");

        int fd[2];
        pid_t p1, p2;
        if (pipe(fd) < 0) {
            printf("Erro ao criar pipe\n");
            return 1;
        }

        p1 = fork();

        if (p1 == 0) { // Processo filho
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);

            char* args[100];
            int argc;
            parseCommand(cmd1, args, &argc);

            Command *command = newCommand(cmd1);
            logCommand(command);
            if (execvp(args[0], args) < 0) {
                printf("Erro ao executar comando\n");
                exit(0);
            }
        } else {
            p2 = fork();

            if (p2 == 0) { // Processo filho
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);

                char* args[100];
                int argc;
                parseCommand(cmd2, args, &argc);

                Command *command = newCommand(cmd2);
                logCommand(command);
                if (execvp(args[0], args) < 0) {
                    printf("Erro ao executar comando\n");
                    exit(0);
                }
            } else {
                close(fd[0]);
                close(fd[1]);
                wait(NULL);
                wait(NULL);
            }
        }
    } else {
        char* args[100];
        int argc;
        parseCommand(input, args, &argc);
        
        
    }
    return 0;
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

        processOperator(input);
    }

    return 0;
}

