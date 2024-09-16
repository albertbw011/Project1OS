#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include "jobs.h"

Job *job_list = NULL; 	
int next_jid = 1;
pid_t pid1 = -1;
pid_t pid2 = -1;

void init_command(Command *cmd) {
    cmd->command = NULL;
    cmd->args = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->error_file = NULL;
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
void print_full_command(Command *cmd) {
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
Standard print function
Prints only original command (not supported for pipes)
*/
void print_command(Command *cmd) {
	for (int i = 0; cmd->args[i] != NULL; i++) 
		printf("%s", cmd->args[i]);
	printf("\n");
}

/*
Initializes new Job struct
*/
Job *create_job(Command *command, int background) {
	Job *job = malloc(sizeof(Job));
	job->jid = next_jid++;
	job->pgid = 0;
	job->command = command;
	job->command2 = NULL;
	job->status = RUNNING;
	job->next = NULL;
	job->background = background;
	
	return job;
}

Job *create_pipe_job(Command *cmd1, Command *cmd2, int background) {
	Job *job = malloc(sizeof(Job));
	job->jid = next_jid++;
	job->pgid = 0;
	job->command = cmd1;
	job->command2 = cmd2;
	job->status = RUNNING;
	job->next = NULL;
	job->background = background;

	return job;
}

void add_job(Job *new_job) {
	// add job to the end of the linked list
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

			if (current->command2 != NULL) free_command(current->command2);
			free_command(current->command);
			free(current);
			return;
		}

		prev = current;
		current = current->next;
	}
}

void list_jobs() {
	Job *current = job_list;

	while (current != NULL) {
		printf("[%d] %s %s\t", current->jid, current->background ? "-" : "+", (current->status == RUNNING) ? "Running" : "Stopped");
		print_command(current->command);
		current = current->next;
	}
}

Job *find_job_by_jid(int jid) {
	Job *current = job_list;

	while (current != NULL) {
		if (current->jid == jid) {
			return current;
		} 
		current = current->next;
		
	}

	return NULL;
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

/*
Update necessary files based on input, output, error in Command struct
*/
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
        fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (dup2(fd, STDOUT_FILENO) < 0) {
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (cmd->error_file != NULL) {
        fd = open(cmd->error_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (dup2(fd, STDERR_FILENO) < 0) {
            close(fd);
            return -1;
        }
        close(fd);
    }

    return 0;
}

/*
Executes job commands with execvp()
Handles cases with pipes and standard commands
*/
void execute_job(Job *job) {
	if (job->command2 == NULL) {
		// execute standard command with no pipe
		Command *cmd = job->command;
		pid1 = fork();

		if (pid1 < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		} else if (pid1 == 0) {
			setpgid(0, 0);
			if (handle_redirection(cmd) == 0) {
				if (execvp(cmd->command, cmd->args) == -1)
					exit(EXIT_FAILURE);
			} 
		} else {
			setpgid(pid1, pid1);   

			if (job->background) {
				printf("[%d] %d\n", job->jid, job->pgid);
			} else { 
				int status;
				waitpid(pid1, &status, WUNTRACED);
				if (WIFSTOPPED(status)) {
					job->status = STOPPED;
				} else {
					remove_job(job->pgid);
				}
			}
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

		pid1 = fork();
		if (pid1 < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		} else if (pid1 == 0) {
			setpgid(0, 0);
			close(pipefd[0]);
			dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[1]);
			if (execvp(left_command->command, left_command->args) == -1) {
				printf("%s: command not found\n", left_command->command);
				exit(EXIT_FAILURE);
			}
		} 

		pid2 = fork();
		if (pid2 < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		} else if (pid2 == 0) {
			setpgid(0, pid1);
			close(pipefd[1]);
			dup2(pipefd[0], STDIN_FILENO);
			close(pipefd[0]);
			if (execvp(right_command->command, right_command->args) == -1) {
				printf("%s: command not found\n", right_command->command);
				exit(EXIT_FAILURE);
			}
		}

		close(pipefd[0]);
		close(pipefd[1]);
		setpgid(pid1, pid1);
		setpgid(pid2, pid1);

		job->pgid = pid1;
		add_job(job);

		waitpid(pid1, &status, WUNTRACED);
		waitpid(pid2, &status, WUNTRACED);

		if (!WIFSTOPPED(status)) {
			remove_job(job->pgid);
		}
	}
}