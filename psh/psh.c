#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "psh.h"

// Macros for reading input
#define PSH_RL_BUFSIZE 1024
#define PSH_TOK_BUFSIZE 64
#define PSH_TOK_DELIM " \t\r\n\a"

// global variable for built-ins
char *builtin_str[] = {"cd", "help", "exit"};

int main(int argc, char **argv) {
    psh_loop();
    return EXIT_SUCCESS;
}

void psh_loop() {
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = psh_read_line();
        args = psh_split_line(line);
        status = psh_execute(args);

        free(line);
        free(args);
    } while (status);
}

char *psh_read_line(void) {
    int bufsize = PSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "psh: allocation error\n");
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
            bufsize += PSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "psh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **psh_split_line(char *line) {
    int bufsize = PSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens) {
        fprintf(stderr, "psh: allocation error");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, PSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += PSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                fprintf(stderr, "psh: allocation error");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, PSH_TOK_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}

int psh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("psh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("psh");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int (*builtin_func[])(char **) = {
    &psh_cd,
    &psh_help,
    &psh_exit,
};

int psh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int psh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "psh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("psh");
        }
    }
    return 1;
}

int psh_help(char **args) {
    int i;
    printf("This is Pranav Reddy's shell.\n");
    printf("It is based on Stephen Brennan's lsh tutorial\n");
    printf("You can find a link to it here: https://brennan.io/2015/01/16/write-a-shell-in-c/\n\n");
    printf("The following commands are built-in:\n");
    for (i = 0; i < psh_num_builtins(); i++) {
        printf("	%s\n", builtin_str[i]);
    }
    printf("Use the man command for information on other programs.\n");
    return 1;
}

int psh_exit(char **args) {
    return 0;
}

int psh_execute(char **args) {
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < psh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return psh_launch(args);
}