#include <fcntl.h>
#include "parsing.c"

typedef enum { RUNNING, STOPPED, BACKGROUND } job_status;

typedef struct {
	int jid;
	pid_t pgid;
	Command *command;
	job_status status;
	struct Job *next;
} Job;

Job *job_list = NULL; 	
int next_jid = 1;

void add_job(pid_t pgid, Command *cmd, job_status status) {
	Job *new_job = malloc(sizeof(Job));
	new_job->jid = next_jid++;
	new_job->pgid = pgid;
	new_job->status = status;
	new_job->command = cmd;
	new_job->next = job_list;
	
	// add job to the end of the list
	if (job_list == NULL) {
		job_list = new_job;
	} else {
		Job *current = job_list;
		while (current->next != NULL) current = current->next;
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
		printf("");
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
