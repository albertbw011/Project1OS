#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE_CHAR 2000
#define DELIM " \t\n"

typedef struct {
    char *command;
    char **args;
    char *input_file;
    char *output_file;
    char *error_file;
    int background;
} Command;

void init_command(Command *cmd) {
    cmd->command = NULL;
    cmd->args = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->error_file = NULL;
    cmd->background = 0;
}

void free_command(Command *cmd) {
    if (cmd->command) free(cmd->command);
    if (cmd->args) {
        for (int i = 0; cmd->args[i] != NULL; i++) {
            free(cmd->args[i]);
        }
        free(cmd->args);
    }
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
    if (cmd->error_file) free(cmd->error_file);
}

void execute_command(Command *cmd) {
    pid_t pid = fork();

    if (pid == 0) {

    }
}

void handle_redirection(char **file, char *token, int*curr_len) {
    if (token != NULL) {
        *curr_len += strlen(token);
        *file = strdup(token);
    }
}

void parse_redirection(char *token, Command *cmd, int *curr_len) {
    token = strtok(NULL, DELIM);
    
    if (strcmp(token, "<") == 0) {
        handle_redirection(&cmd->input_file, token, curr_len);
    } else if (strcmp(token, ">") == 0) {
        handle_redirection(&cmd->output_file, token, curr_len);
    } else if (strcmp(token, "2>") == 0) {
        handle_redirection(&cmd->error_file, token, curr_len);
    }
}

/*
Parses input string into 2D character array
*/
Command* parse_input(char *input) {
    char **tokens = malloc(MAX_LINE_CHAR / 2 * sizeof(char* )); // dynamic tokens array
    char *token;
    int i = 0;
    int curr_len = 0; 
    int arg_index = 0;
    int args_size = 8;
    Command* cmd;
    init_command(cmd);

    cmd->args = (char **) malloc(args_size * sizeof(char* ));
    if (cmd->args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    token = strtok(input, DELIM);

    while (token != NULL && curr_len < MAX_LINE_CHAR) {
        curr_len += strlen(token);

        if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, "2>") == 0) {
            parse_redirection(token, cmd, &curr_len);
        } else {
            if (cmd->command == NULL) {
                cmd->command = strdup(token);
            }

            if (arg_index >= args_size - 1) {
                args_size *= 2;
                cmd->args = (char **) realloc(cmd->args, args_size * sizeof(char *));
            } 

            cmd->args[arg_index++] = strdup(token);
        }

        token = strtok(NULL, DELIM);
    }
}

int main() {
    char *command;
    char **tokens;
    pid_t curr_pid;
    int status;

    while (1) {
        command = readline("# "); 

        if (command == NULL) {
            printf("\n");
            break;
        } 
        
        Command* cmd = parse_input(command);
        curr_pid = fork();

        if (curr_pid == 0) {
            if (execvp(tokens[0], tokens) == -1) {
                continue;
            }
        } else {
            waitpid(curr_pid, &status, 0);
        }

        free_command(command);
    }
    
    return 0;
}