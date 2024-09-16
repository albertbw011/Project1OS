#ifndef PARSING_H
#define PARSING_H

/*
Includes Command struct definition and parsing logic
*/

#define DELIM " \t\n"

typedef struct Command Command;
typedef struct Job Job;

/*
Update command struct based on the file redirection symbol
*/
void parse_redirection(Command *cmd, char *token, char **save_ptr);

/*
Parses input string into Job object
*/
Job *parse_input(char *input);

/*
Convert standard string input into a Command struct
*/
Command *parse_command(char *input);

#endif