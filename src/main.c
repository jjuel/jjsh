#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define JJSH_RL_BUFSIZE 1024
#define JJSH_TOK_BUFSIZE 64
#define JJSH_TOK_DELIM " \t\r\n\a"

void jjsh_loop(void);
char *jjsh_read_line(void);
char **jjsh_split_line(char *line);
int jjsh_execute(char **args);
int jjsh_cd(char **args);
int jjsh_help(char **args);
int jjsh_exit(char **args);

int main(void) {
  jjsh_loop();

  return EXIT_SUCCESS;
}

void jjsh_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = jjsh_read_line();
    args = jjsh_split_line(line);
    status = jjsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

char *jjsh_read_line(void) {
  int bufsize = JJSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "jjsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buffer[position] = '\0';

      return buffer;
    } else {
      buffer[position] = c;
    }

    position++;

    if (position >= bufsize) {
      bufsize += JJSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "jjsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

char **jjsh_split_line(char *line) {
  int bufsize = JJSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token;

  if (!tokens) {
    fprintf(stderr, "jjsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, JJSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += JJSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens) {
        fprintf(stderr, "jjsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, JJSH_TOK_DELIM);
  }

  tokens[position] = NULL;

  return tokens;
}

int jjsh_launch(char **args) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("jjsh");
    }
    exit(EXIT_SUCCESS);
  } else if (pid < 0) {
    // Forking error
    perror("jjsh");
  } else {
    // Parent Process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

char *builtin_str[] = {"cd", "help", "exit"};
int (*builtin_func[])(char **) = {&jjsh_cd, &jjsh_help, &jjsh_exit};
int jjsh_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int jjsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "jjsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("jjsh");
    }
  }

  return 1;
}

int jjsh_help(char **args) {
  int i;
  printf("Jordan Juel's JJSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < jjsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");

  return 1;
}

int jjsh_exit(char **args) { return 0; }

int jjsh_execute(char **args) {
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < jjsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return jjsh_launch(args);
}
