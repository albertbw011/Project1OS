#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

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

int handle_redirection(Command *cmd);

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
        for (int i = 0; cmd->args[i] != NULL; i++)
            free(cmd->args[i]);
        free(cmd->args);
    }
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
    if (cmd->error_file) free(cmd->error_file);
}

/*
For debugging purposes
*/
void print_command(Command *cmd) {
    if (cmd->command) printf("command: %s\n", cmd->command);
    if (cmd->args) {
        for (int i = 0; cmd->args[i] != NULL; i++) 
            printf("arg%d: %s\n", i, cmd->args[i]);
    }
    if (cmd->input_file) printf("input file: %s\n", cmd->input_file);
    if (cmd->output_file) printf("output file: %s\n", cmd->output_file);
    if (cmd->error_file) printf("error file: %s\n", cmd->error_file);
}

void execute_command(Command *cmd) {
    pid_t pid = fork();
    int status;

    if (pid == 0) {
        if (handle_redirection(cmd) == 0) {
            if (execvp(cmd->command, cmd->args) == -1) {
                printf("\n");
            }
        }
    } else {
        waitpid(pid, &status, 0);
    }
}

int handle_redirection(Command *cmd) {
    int fd;

    if (cmd->input_file != NULL) {
        fd = open(cmd->input_file, O_RDONLY);
        if (dup2(fd, STDIN_FILENO) < 0) {
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (cmd->output_file != NULL) {
        fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dup2(fd, STDOUT_FILENO) < 0) {
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (cmd->error_file != NULL) {
        fd = open(cmd->error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dup2(fd, STDERR_FILENO) < 0) {
            close(fd);
            return -1;
        }
        close(fd);
    }

    return 0;
}

/*
Parses input string into 2D character array
*/
Command* parse_input(char *input) {
    char *token;
    char *save_ptr;
    int i = 0;
    int curr_len = 0; 
    int arg_index = 0;
    int args_size = 8;
    Command *cmd = (Command* ) malloc(sizeof(Command));
    init_command(cmd);

    cmd->args = (char **) malloc(args_size * sizeof(char *));
    token = strtok(input, DELIM);

    while (token != NULL && curr_len < MAX_LINE_CHAR) {
        curr_len += strlen(token);

        if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, "2>") == 0) {
            char *file_name = strtok(NULL, DELIM);
    
            if (file_name != NULL) {
                if (strcmp(token, "<") == 0) 
                    cmd->input_file = strdup(file_name);
                else if (strcmp(token, ">") == 0) {
                    cmd->output_file = strdup(file_name);
                } else if (strcmp(token, "2>") == 0)
                    cmd->error_file = strdup(file_name);

                curr_len += strlen(file_name);
            }
        } else {
            if (cmd->command == NULL)
                cmd->command = strdup(token);

            if (arg_index >= args_size - 1) {
                args_size *= 2;
                cmd->args = (char **) realloc(cmd->args, args_size * sizeof(char *));
            } 

            cmd->args[arg_index++] = strdup(token);
        }

        token = strtok(NULL, DELIM);
    }

    cmd->args[arg_index] = NULL;
    return cmd;
}

int main() {
    char *command;

    while (1) {
        command = readline("# "); 

        if (command == NULL) {
            printf("\n");
            break;
        } 

        Command *cmd = parse_input(command);
        execute_command(cmd);
        free_command(cmd);
    }
    
    return 0;
}