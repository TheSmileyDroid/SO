#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

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
    int ret = execvp(args[0], args);
    if (ret != 0) {
      exit(-1);
    }
    exit(0);
  } else {
    int status;
    wait(&status);
    if (status != 0) {
      // printf("Command failed: %s \n", args[0]);
      exit(-1);
    }
    exit(0);
  }
}

enum {
  RUN = 0,
  PIPE = 1,
  AND = 2,
  OR = 3,
  BACKGROUND = 4,
};

void divide_by(char *args[100], char *args1[100], char *args2[100], int pos) {
    int i = 0;
    while (i < pos) {
      args1[i] = args[i];
      i++;
    }
    args1[i] = NULL;

    i = 0;
    int j = pos;
    while (args[j + 1] != NULL) {
      args2[i] = args[j + 1];
      j++;
      i++;
    }
    args2[i] = NULL;
}

int separate(char *args[100], char *args1[100], char *args2[100]) {
  int pipePos = get_pos(args, "|");
  int andPos = get_pos(args, "&&");
  int orPos = get_pos(args, "||");
  int backgroundPos = get_pos(args, "&");

  if (pipePos != -1) {
    divide_by(args, args1, args2, pipePos);
    return PIPE;
  } else if (orPos != -1) {
    divide_by(args, args1, args2, orPos);
    return OR;
  } else if (andPos != -1) {
    divide_by(args, args1, args2, andPos);
    return AND;
  } else if (backgroundPos != -1) {
    int i = 0;
    while (args[i] != NULL) {
      args1[i] = args[i];
      i++;
    }
    args1[backgroundPos] = NULL;
    return BACKGROUND;
  } else {
    return RUN;
  }
}


void process(char *args[100]) {
  /*
  pid_t pid = getpid();
  printf("PID: %d: args: ", pid);
  int i = 0;
  while (args[i] != NULL) {
    printf("%s, ", args[i]);
    i += 1;
  }
  printf("\n");
  */
  char *args1[100] = {NULL};
  char *args2[100] = {NULL};

  int state = separate(args, args1, args2);
  //printf("PID: %d: State: %d\n", pid, state);

  if (state == RUN) {
    run(args);
    return;
  } else if (state == PIPE) {
    int fd[2];
    pipe(fd);

    pid_t p1, p2;
    p1 = fork();

    if (p1 == 0) {
      close(fd[0]);
      dup2(fd[1], STDOUT_FILENO);
      close(fd[1]);
      process(args1);
      exit(0);
    } else {
      p2 = fork();
      if (p2 == 0) {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        process(args2);
        exit(0);
      } else {
        close(fd[0]);
        close(fd[1]);
        wait(NULL);
        wait(NULL);
      }
    }
  } else if (state == OR) {
    pid_t pid = fork();
    if (pid == 0) {
      process(args1);
      exit(-1);
    } else {
      int status;
      wait(&status);
      if (status != 0) {
        process(args2);
        exit(-1);
      }
    }
  } else if (state == AND) {
    pid_t pid = fork();
    if (pid == 0) {
      process(args1);
      exit(-1);
    } else {
      int status;
      wait(&status);
      if (status == 0) {
        process(args2);
      } else {
        exit(-1);
      }
    }
  } else if (state == BACKGROUND) {
    pid_t pid = fork();
    if (pid == 0) {
      process(args1);
      exit(0);
    }
  }
}

void get_input(char *input) {
  printf("> ");
  fgets(input, 100, stdin);
  int len = strlen(input);
  if (input[len - 1] == '\n') {
    input[len - 1] = '\0';
  }
}

void parse_input(char *input, char *args[100], int *argc) {
  bool in_quotes = false;
  char *arg_start = input;

  for (char *p = input; *p; p++) {
    if (*p == '"' || *p == '\'') {
      in_quotes = !in_quotes;
    } else if (*p == ' ' && !in_quotes) {
      *p = '\0';
      args[(*argc)++] = arg_start;
      arg_start = p + 1;
    }
  }

  args[(*argc)++] = arg_start;
  args[*argc] = NULL;

  for (int i = 0; i < *argc; i++) {
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
}

void process_input(char *args[100]) {
  pid_t pid = fork();

  if (pid == 0) {
    process(args);
    exit(0);
  } else {
    wait(NULL);
  }
}

int main() {
  while (true) {
    char input[100];
    get_input(input);

    if (strcmp(input, "exit") == 0) {
      break;
    }

    char *args[100];
    int argc = 0;

    parse_input(input, args, &argc);

    process_input(args);
  }

  return 0;
}
