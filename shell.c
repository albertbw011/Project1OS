#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE_CHAR 2000
#define DELIM " \t\n"

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
    pid_t curr_pid;
    int status;

    while (1) {
        command = readline("yash$ "); 

        if (command == NULL) {
            printf("\n");
            continue;
        } 
        
        tokens = parse_input(command);
        curr_pid = fork();

        if (curr_pid == 0) {
            if (execvp(tokens[0], tokens) == -1) {
                continue;
            }
        } else {
            waitpid(curr_pid, &status, 0);
        }

        free(command);
    }
    return 0;
}