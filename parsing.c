#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "parsing.h"
#include "jobs.h"

// extern char *strdup(const char*);
// extern char *strtok_r(char *str, const char *delim, char **saveptr);

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
Parses input string into Job object
*/
Job *parse_input(char *input) {
	char *pipe_pos = strchr(input, '|');
	int background = (strchr(input, '&') != NULL) ? 1 : 0;
	char *input_copy = strdup(input);

	if (pipe_pos != NULL) {
		*pipe_pos = '\0';
		Command *left_command = parse_command(input);
		Command *right_command = parse_command(pipe_pos+1);
		return create_pipe_job(left_command, right_command, input_copy, background);
	} else {
		Command *cmd = parse_command(input);
		return create_job(cmd, input_copy, background);
	}
}

/*
Convert standard string input into a Command struct
*/
Command *parse_command(char *input) {
	char *token;
	char *save_ptr;
	int arg_index = 0;
	int args_size = 8;
	Command *cmd = (Command* ) malloc(sizeof(Command));
	init_command(cmd);

	cmd->args = (char** ) malloc(args_size * sizeof(char* ));
	cmd->commandstring = strdup(input);
	token = strtok_r(input, DELIM, &save_ptr);

	while (token != NULL) {
        if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, "2>") == 0) {
            parse_redirection(cmd, token, &save_ptr);
        } else {
            if (arg_index >= args_size - 1) {
                args_size *= 2;
                cmd->args = (char **) realloc(cmd->args, args_size * sizeof(char *));
            } 

			if (strcmp(token, "&") != 0) {
            	cmd->args[arg_index++] = strdup(token);
			}
        }

        token = strtok_r(NULL, DELIM, &save_ptr);
    }
    
    cmd->args[arg_index] = NULL;
    return cmd;
}