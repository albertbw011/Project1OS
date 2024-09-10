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
Prints each element of Command object
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
            execvp(cmd->command, cmd->args);
        } 
        exit(EXIT_SUCCESS);
    } else {
        waitpid(pid, &status, 0);
    }
}

int handle_redirection(Command *cmd) {
    int fd;

    if (cmd->input_file != NULL) {
        fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            perror(cmd->input_file);
            return -1;
        }
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

void parse_redirection(Command *cmd, char *token, char **save_ptr) {
    char *file_name = strtok_r(NULL, DELIM, save_ptr);
    
    if (strcmp(token, "<") == 0) 
        cmd->input_file = strdup(file_name);
    else if (strcmp(token, ">") == 0) 
        cmd->output_file = strdup(file_name);
    else if (strcmp(token, "2>") == 0)
        cmd->error_file = strdup(file_name);
}

/*
Parses input string into 2D character array
*/
Command* parse_input(char *input) {
    char *token;
    char *save_ptr;
    int arg_index = 0;
    int args_size = 8;
    Command *cmd = (Command* ) malloc(sizeof(Command));
    init_command(cmd);

    cmd->args = (char **) malloc(args_size * sizeof(char *));
    token = strtok_r(input, DELIM, &save_ptr);

    while (token != NULL) {
        if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, "2>") == 0) {
            parse_redirection(cmd, token, &save_ptr);
        } else {
            if (cmd->command == NULL)
                cmd->command = strdup(token);

            if (arg_index >= args_size - 1) {
                args_size *= 2;
                cmd->args = (char **) realloc(cmd->args, args_size * sizeof(char *));
            } 
            cmd->args[arg_index++] = strdup(token);
        }

        token = strtok_r(NULL, DELIM, &save_ptr);
    }
    
    cmd->args[arg_index] = NULL;
    return cmd;
}

Command** parse_pipe_input(char *left_command, char *right_command) {
    Command **commands = (Command **) malloc(2 * sizeof(Command *));
    commands[0] = parse_input(left_command);
    commands[1] = parse_input(right_command);
    return commands;
}

int main() {
    char *command;

    while (1) {
        command = readline("# "); 

        if (command == NULL) {
            printf("\n");
            break;
        } 

        char *pipe_pos = strstr(command, "|");
        
        if (pipe_pos != NULL) {
            *pipe_pos = '\0';
            char *left_command = command;
            char *right_command = pipe_pos + 1;
            Command **commands = parse_pipe_input(left_command, right_command);
            free_command(commands[0]);
            free_command(commands[1]);
            free(commands);
        } else {
            Command *cmd = parse_input(command);
            execute_command(cmd);
            free_command(cmd);
        }
    }
    
    return 0;
}