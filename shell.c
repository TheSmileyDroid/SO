#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * Retorna a posição de um argumento
 * 
 * args argumentos
 * str argumento a ser procurado
 */
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

/**
 * Executa um comando sem operadores
 * 
 * args argumentos
 */
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
  REDIRECT_OUT = 5,
  REDIRECT_IN = 6,
  APPEND = 7,
};

/**
 * Divide os argumentos em duas partes na posição pos
 * 
 * args argumentos
 * args1 primeira parte
 * args2 segunda parte
 * pos posição
 */
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

/**
 * Divide os argumentos em duas partes
 * args: argumentos
 * args1: primeira parte
 * args2: segunda parte
 *
 * Retorna o o tipo de operador ou RUN se não tiver operador
 *
 * 0: RUN
 * 1: PIPE
 * 2: AND
 * 3: OR
 * 4: BACKGROUND
 * 5: REDIRECT_OUT
 * 6: REDIRECT_IN
 * 7: APPEND
 */
int separate(char *args[100], char *args1[100], char *args2[100]) {
  int pipePos = get_pos(args, "|");
  int andPos = get_pos(args, "&&");
  int orPos = get_pos(args, "||");
  int backgroundPos = get_pos(args, "&");
  int redirectOutPos = get_pos(args, ">");
  int redirectInPos = get_pos(args, "<");
  int appendPos = get_pos(args, ">>");


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
  } else if (appendPos != -1) {
    divide_by(args, args1, args2, appendPos);
    return APPEND;
  } else if (redirectOutPos != -1) {
    divide_by(args, args1, args2, redirectOutPos);
    return REDIRECT_OUT;
  } else if (redirectInPos != -1) {
    divide_by(args, args1, args2, redirectInPos);
    return REDIRECT_IN;
  } else {
    return RUN;
  }
}

/**
 * Checa se o comando tem operadores e executa
 * 
 * args argumentos
 */
void process(char *args[100]) {
  char *args1[100] = {NULL};
  char *args2[100] = {NULL};

  int operator = separate(args, args1, args2);

  if (operator == RUN) { // comando sem operadores
    run(args);
  } else if (operator == PIPE) { // comando com pipe
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
  } else if (operator == OR) { // comando com ou
    pid_t pid = fork();
    if (pid == 0) {
      process(args1);
      exit(-1);
    } else {
      int status;
      wait(&status);
      if (status != 0) { // se o comando anterior terminou com erro
        process(args2);
        exit(-1);
      }
    }
  } else if (operator == AND) { // comando com e
    pid_t pid = fork();
    if (pid == 0) {
      process(args1);
      exit(-1);
    } else {
      int status;
      wait(&status);
      if (status == 0) { // se o comando anterior terminou com sucesso
        process(args2);
      } else {
        exit(-1);
      }
    }
  } else if (operator == BACKGROUND) { // comando com &
    pid_t pid = fork();
    if (pid == 0) {
      process(args1);
      exit(0);
    }
  } else if (operator == REDIRECT_OUT) { // comando com redirecionamento de saída
    pid_t pid = fork();
    if (pid == 0) {
      // cria o arquivo se não existir e apaga o conteúdo
      int fd = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      dup2(fd, STDOUT_FILENO);
      close(fd);
      process(args1);
      exit(0);
    }
  } else if (operator == REDIRECT_IN) { // comando com redirecionamento de entrada
    pid_t pid = fork();
    if (pid == 0) {
      // abre o arquivo para leitura
      int fd = open(args2[0], O_RDONLY);
      dup2(fd, STDIN_FILENO);
      close(fd);
      process(args1);
      exit(0);
    }
  } else if (operator == APPEND) { // comando com append
    pid_t pid = fork();
    if (pid == 0) {
      // cria o arquivo se não existir e mantém o conteúdo existente
      int fd = open(args2[0], O_WRONLY | O_CREAT | O_APPEND, 0644);
      dup2(fd, STDOUT_FILENO);
      close(fd);
      process(args1);
      exit(0);
    }
  }
}

/**
 * Recebe o input do usuário
 * 
 * input input do usuário
 */
void get_input(char *input) {
  printf("> "); // prompt do shell
  fgets(input, 100, stdin);
  int len = strlen(input);
  if (input[len - 1] == '\n') {
    input[len - 1] = '\0';
  }
}

/**
  * Divide o input em argumentos
  * 
  * input input do usuário
  * args argumentos
  * argc quantidade de argumentos
  */
void parse_input(char *input, char *args[100], int *argc) {
  bool in_quotes = false; // se está dentro de aspas
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
  args[*argc] = NULL; // último argumento é NULL

  for (int i = 0; i < *argc; i++) { // remove aspas dos argumentos
    char *arg = args[i];
    int len = strlen(arg);
    if (arg[0] == '"' && arg[len - 1] == '"') { // remove aspas duplas
      arg[len - 1] = '\0';
      args[i] = arg + 1;
    } else if (arg[0] == '\'' && arg[len - 1] == '\'') { // remove aspas simples
      arg[len - 1] = '\0';
      args[i] = arg + 1;
    }
  }
}


/**
 * Roda o comando com os argumentos
 * Utilizando um fork para rodar em paralelo
 * e evitar que o shell seja fechado 
 *
 * args argumentos
 */
void process_input(char *args[100]) {
  pid_t pid = fork();

  if (pid == 0) {
    process(args);
    exit(0);
  } else {
    wait(NULL);
  }
}

/**
 * Função principal
 */
int main() {
  while (true) {
    char input[100];
    get_input(input);

    if (strcmp(input, "exit") == 0) { // comando para sair
      break;
    }

    char *args[100];
    int argc = 0;

    parse_input(input, args, &argc);

    process_input(args);
  }

  return 0;
}
