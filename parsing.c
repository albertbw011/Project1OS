#include "yash.c"

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
