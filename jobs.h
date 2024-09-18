#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <unistd.h>

#define MAX_JOBS 20

typedef struct Command {
	char *commandstring;
    char **args;
    char *input_file;
    char *output_file;
    char *error_file;
} Command;

void init_command(Command *cmd);

void free_command(Command *cmd);

typedef enum { RUNNING, STOPPED } job_status;

typedef struct CompletedJob {
	int jid; // job id of finished job
	char *command; // job command
} CompletedJob;

typedef struct Job {
	int jid; // job id
	pid_t pgid; // process group id
	char *jobstring; // user input
	Command *command; // job command (1st command if there is a pipe)
	Command *command2; // for pipes 
	job_status status; // RUNNING or STOPPED
	int background; // 1 for bg, 0 for fg
	struct Job *next; // pointer to next job in list
} Job;

extern Job *job_list;
extern CompletedJob completed_jobs[MAX_JOBS];
extern int completed_job_count;

/*
Initializes new Job struct
*/
Job *create_job(Command *command, char *commandstring, int background);

Job *create_pipe_job(Command *cmd1, Command *cmd2, char *commandstring, int background);

void print_job(Job *job);

void free_job(Job *job);

void add_job(Job *new_job);

void remove_job(pid_t pgid);

int find_next_job_id();

void list_jobs();

/*
Function for handling fg
*/
void handle_fg();

/*
Function for handling bg
*/
void handle_bg();

/*
Prints all completed jobs
*/
void print_completed_jobs();

Job *find_job_by_pgid(pid_t pgid);

Job *get_foreground_job();

/*
Update necessary files based on input, output, error in Command struct
*/
void handle_redirection(Command *cmd, int output_fd);

/*
Executes job commands with execvp()
Handles cases with pipes and standard commands
*/
void execute_job(Job *job);

#endif