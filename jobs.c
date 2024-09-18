#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "jobs.h"

Job *job_list = NULL;
CompletedJob completed_jobs[MAX_JOBS];
int completed_job_count = 0;

void init_command(Command *cmd) {
	cmd->commandstring = NULL;
    cmd->args = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->error_file = NULL;
}

void free_command(Command *cmd) {
	if (cmd == NULL) return;
	if (cmd->commandstring) free(cmd->commandstring);
    if (cmd->args) {
        for (int i = 0; cmd->args[i] != NULL; i++)
            free(cmd->args[i]);
        free(cmd->args);
    }
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
    if (cmd->error_file) free(cmd->error_file);
	free(cmd);
}

void print_command(Command *cmd) {
	printf("commandstring: %s\n", cmd->commandstring);
	for (int i = 0; cmd->args[i] != NULL; i++)
		printf("arg[%d]: %s\n", i, cmd->args[i]);
	printf("input_file: %s\n", cmd->input_file != NULL ? cmd->input_file : "None");
	printf("output_file: %s\n", cmd->output_file != NULL ? cmd->output_file : "None");
	printf("error_file: %s\n", cmd->error_file != NULL ? cmd->error_file : "None");
}

/*
Initializes new Job struct
*/
Job *create_job(Command *command, char *jobstring, int background) {
	Job *job = malloc(sizeof(Job));
	job->pgid = 0;
	job->jobstring = jobstring;
	job->command = command;
	job->command2 = NULL;
	job->status = RUNNING;
	job->next = NULL;
	job->background = background;
	
	return job;
}

void print_job(Job *job) {
	printf("jid: %d\n", job->jid);
	printf("pgid: %d\n", job ->pgid);
	printf("jobstring: %s\n", job->jobstring);
	printf("command1string: %s\n", job->command->commandstring);
	if (job->command2) printf("command2string: %s\n", job->command2->commandstring);
	printf("status: %s\n", (job->status == RUNNING) ? "Running" : "Stopped");
	printf("next: %s\n", (job->next) ? "yes" : "no");
	printf("background: %d\n", job->background);
}

Job *create_pipe_job(Command *command1, Command *command2, char *commandstring, int background) {
	Job *job = create_job(command1, commandstring, background);
	job->command2 = command2;
	
	return job;
}

/*
Frees job from memory
Also frees pointer to job
*/
void free_job(Job *job) {
	if (job == NULL) return;
	if (job->command) free_command(job->command); 
	if (job->command2) free_command(job->command2);
	if (job->jobstring) free(job->jobstring);
	free(job);
}

// add job to the end of the linked list
void add_job(Job *new_job) {
	new_job->jid = find_next_job_id(); 

	if (job_list == NULL) {
		job_list = new_job;
	} else {
		Job *current = job_list;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = new_job;
	}
}

/*
Removes job from job list with given pgid
Deletes job from memory
*/
void remove_job(pid_t pgid) {
	Job *prev = NULL;
	Job *current = job_list;

	while (current != NULL) {
		if (current->pgid == pgid) {
			if (prev == NULL) {
				job_list = current->next;
			} else {
				prev->next = current->next;
			}
			
		    free_job(current);
			return;
		}

		prev = current;
		current = current->next;
	}
}

/*
Finds lowest available job number
*/
int find_next_job_id() {
	int jid = 1;
	Job *current = job_list;

	while (current != NULL) {
		if (current->jid >= jid) {
			jid = current->jid + 1;
		} else {
			current = current->next;
		}
	}

	return jid;
}


/*
Returns most recent stopped/background job in the job list
*/
Job *most_recent_job() {
	Job *current = job_list;
	Job *most_recent = NULL;

	while (current != NULL) {
		if (current->status == STOPPED || current->background) {
			most_recent = current;
		}
		current = current->next;
	}

	return most_recent;
}

/*
Returns most recent stopped job in the job list
*/
Job *most_recent_stopped_job() {
	Job *current = job_list;
	Job *most_recent = NULL;

	while (current != NULL) {
		if (current->status == STOPPED) {
			most_recent = current;
		}
		current = current->next;
	}

	return most_recent;
}

void list_jobs() {
	Job *current = job_list;

	while (current != NULL) {
		printf("[%d]%s  %s\t\t%s\n", 
		current->jid, 
		current->next ? "-" : "+", 
		(current->status == RUNNING) ? "Running" : "Stopped", 
		current->jobstring);
		current = current->next;
	}
}

/*
Bring most recent stopped/background job to foreground
*/
void handle_fg() {
	Job *job = most_recent_job();
	
	if (job) {
		// send SIGCONT to most recent job
		kill(-job->pgid, SIGCONT);

		// give terminal control back to job and set to running
		tcsetpgrp(STDIN_FILENO, job->pgid);
		job->status = RUNNING;
		job->background = 0;

		// print out command again
		char *print_cmd = strdup(job->jobstring);
		int len = strlen(print_cmd);

		if (len > 0 && print_cmd[len-1] == '&') {
			print_cmd[len-1] = '\0';
		}

		printf("%s\n", print_cmd);
		free(print_cmd);

		int status;
		waitpid(-job->pgid, &status, WUNTRACED);

		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			remove_job(job->pgid);
		} else if (WIFSTOPPED(status)) {
			job->status = STOPPED;
		}

		tcsetpgrp(STDIN_FILENO, getpgrp());
	}
}

/*
Run most recent stopped job in background
*/
void handle_bg() {
	Job *job = most_recent_stopped_job();

	if (job) {
		// send SIGCONT to most recent stopped job
		kill(-job->pgid, SIGCONT);
		job->background = 1;
		job->status = RUNNING;

		// print out command again
		char *print_cmd = strdup(job->jobstring);
		int len = strlen(print_cmd);

		if (len > 0 && print_cmd[len-1] == '&') {
			print_cmd[len-1] = '\0';
		}

		printf("%s\n", print_cmd);
		free(print_cmd);
	} 
}

/*
Prints all completed jobs
*/
void print_completed_jobs() {
	for (int i = 0; i < completed_job_count; i++) {
		printf("[%d]-  Done\t\t%s\n", completed_jobs[i].jid, completed_jobs[i].command);
		free(completed_jobs[i].command);
	}

	completed_job_count = 0;
}

Job *find_job_by_pgid(pid_t pgid) {
	Job *current = job_list;

	while (current != NULL) {
		if (current->pgid == pgid) {
			return current;
		}
		current = current->next;
	}

	return NULL;
}

Job *get_foreground_job() {
	Job *current = job_list;

	while (current != NULL) {
		if (current->background == 0) 
			return current;
		current = current->next;
	}

	return NULL;
}

/*
Update necessary files based on input, output, error in Command struct
*/
void handle_redirection(Command *cmd, int fd_output) {
    int fd;
    if (cmd->input_file != NULL) {
        fd = open(cmd->input_file, O_RDONLY);
		if (fd == -1) {
			exit(EXIT_FAILURE);
		}
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (cmd->output_file != NULL) {
        fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (fd == -1) {
			exit(EXIT_FAILURE);
		}
		dup2(fd, STDOUT_FILENO);
        close(fd);
    } else if (fd_output != -1) {
		dup2(fd_output, STDOUT_FILENO);
	}

    if (cmd->error_file != NULL) {
        fd = open(cmd->error_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (fd == -1) {
			exit(EXIT_FAILURE);
		}
        dup2(fd, STDERR_FILENO);
		close(fd);
    }
}

/*
Executes job commands with execvp()
Handles standard commands and pipes
*/
void execute_job(Job *job) {
	if (job->command2 == NULL) {
		// execute standard command with no pipe
		Command *cmd = job->command;
		pid_t pid = fork();

		if (pid < 0) {
			// fork error
			perror("fork error");
			exit(EXIT_FAILURE);
		} else if (pid == 0) {
			// child process
			setpgid(0, 0);
			handle_redirection(cmd, -1);
			execvp(cmd->args[0], cmd->args);
			exit(EXIT_FAILURE);
		} else {
			// parent process
			setpgid(pid, pid); 
			// printf("bash pid: %d \t yash pid: %d\n", getpgrp(), pid);
			tcsetpgrp(STDIN_FILENO, pid);
			job->pgid = pid;
			add_job(job); 

			if (!job->background) { 
				// wait for child process
				int status;
				waitpid(-pid, &status, WUNTRACED);
				if (WIFEXITED(status) || WIFSIGNALED(status)) {
					remove_job(job->pgid);
				} else if (WIFSTOPPED(status)) {
					job->status = STOPPED;
				}
			}

			tcsetpgrp(STDIN_FILENO, getpgrp());
		}
	} else {
		// exceute pipe command
		Command *left_command = job->command;
		Command *right_command = job->command2;
		int pipefd[2];
		int status;

		if (pipe(pipefd) == -1) {
			perror("pipe error");
			exit(EXIT_FAILURE);
		}

		pid_t pid1 = fork();
		if (pid1 < 0) {
			// fork error
			perror("fork error");
			exit(EXIT_FAILURE);
		} else if (pid1 == 0) {
			// child process
			setpgid(0, 0);

			handle_redirection(left_command, left_command->output_file ? -1 : pipefd[1]);

			// close pipe ends
			close(pipefd[0]);
			close(pipefd[1]);

			// execute command
			execvp(left_command->args[0], left_command->args);
			printf("%s: command not found\n", left_command->args[0]);
			exit(EXIT_FAILURE);
		} 

		pid_t pid2 = fork();
		if (pid2 < 0) {
			// fork error
			perror("fork error");
			exit(EXIT_FAILURE);
		} else if (pid2 == 0) {
			// child process
			setpgid(0, pid1);

			if (right_command->input_file) {
				handle_redirection(right_command, -1);
			} else {
				dup2(pipefd[0], STDIN_FILENO);
				handle_redirection(right_command, STDOUT_FILENO);
			}
			
			close(pipefd[1]);
			close(pipefd[0]);

			// execute command
			execvp(right_command->args[0], right_command->args);
			printf("%s: command not found\n", right_command->args[0]);
			exit(EXIT_FAILURE);
		}

		close(pipefd[0]);
		close(pipefd[1]);
		setpgid(pid1, pid1);
		setpgid(pid2, pid1);
		tcsetpgrp(STDIN_FILENO, pid1);

		job->pgid = pid1;
		add_job(job);

		if (!job->background) { 
			// wait for child process
			int status;
			waitpid(-pid1, &status, WUNTRACED);
			if (WIFEXITED(status) || WIFSIGNALED(status)) {
				remove_job(job->pgid);
			} else if (WIFSTOPPED(status)) {
				job->status = STOPPED;
			}
		}

		tcsetpgrp(STDIN_FILENO, getpgrp());
	}
}