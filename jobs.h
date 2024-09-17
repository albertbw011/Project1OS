#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <unistd.h>

extern pid_t pid1;
extern pid_t pid2;

typedef struct Command {
    char *command;
    char **args;
    char *input_file;
    char *output_file;
    char *error_file;
} Command;

void init_command(Command *cmd);

void free_command(Command *cmd);

/*
Standard print function
Prints only original command (not supported for pipes)
*/
void print_command(Command *cmd);

/*
Prints pipe commands
Includes "\n"
*/
void print_pipe_command(Command *left_command, Command *right_command);

typedef enum { RUNNING, STOPPED } job_status;

typedef struct Job {
	int jid; // job id
	pid_t pgid; // process group id
	Command *command; // job command (1st command if there is a pipe)
	Command *command2; // for pipes 
	job_status status; // RUNNING or STOPPED
	int background; // bg or fg 
	struct Job *next;
} Job;

extern Job *job_list;

/*
Initializes new Job struct
*/
Job *create_job(Command *command, int background);

Job *create_pipe_job(Command *cmd1, Command *cmd2, int background);

void free_job(Job *job);

void add_job(Job *new_job);

void remove_job(pid_t pgid);

void list_jobs();

Job *find_job_by_jid(int jid);

Job *find_job_by_pgid(pid_t pgid);

Job *get_foreground_job();

/*
Update necessary files based on input, output, error in Command struct
*/
int handle_redirection(Command *cmd);

/*
Executes job commands with execvp()
Handles cases with pipes and standard commands
*/
void execute_job(Job *job);

#endif