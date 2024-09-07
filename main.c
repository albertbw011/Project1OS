#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>

#define MAX_LINE_CHAR 2000
#define MAX_TOKEN_LEN 30
#define DELIM " \t\n"

typedef struct { 
    char *command;
    char *args;
    char *input_file;
    char *output_file;
} process;

/*
Parses input string into 2D character array
*/
char** parse_input(char *input) {
    char **tokens = malloc(MAX_LINE_CHAR / 2 * sizeof(char* )); // dynamic tokens array
    char *token;
    int i = 0;
    int curr_len = 0; 

    token = strtok(input, DELIM);

    while (token != NULL && curr_len < MAX_LINE_CHAR) {
        curr_len += strlen(token);
        tokens[i++] = token;
        token = strtok(NULL, DELIM);
    }

    tokens[i] = NULL;
    return tokens;
}

int main() {
    char *command;
    char **tokens;

    while (1) {
        command = readline("yash$ "); 

        if (!command) {
            printf("\n");
        } else {
            tokens = parse_input(command);
        }

        free(command);
    }
    return 0;
}