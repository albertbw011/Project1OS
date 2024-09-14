/*
Includes Command struct definition and parsing logic
*/
#include <stdlib.h>

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

/*
Update command struct based on the file redirection symbol
*/
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
Parses input string into Command object
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

