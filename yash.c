#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include "parsing.h"
#include "jobs.h"

void display_new() {
	rl_replace_line("", 0);
	rl_on_new_line();
	rl_redisplay();
}

/*
Handle signals
*/
void sig_handler(int signo) {
    switch (signo) {
		// Ctrl+C
        case SIGINT:
		{
            Job *foreground = get_foreground_job();
			write(STDOUT_FILENO, "\n", 1);
			if (foreground != NULL) {
				kill(-foreground->pgid, SIGINT);
			} else {
				display_new();
			}
            break;
		}
		
		// Ctrl+Z
        case SIGTSTP:
		{
            Job *foreground = get_foreground_job();
			write(STDOUT_FILENO, "\n", 1);
			if (foreground != NULL) {
				kill(-foreground->pgid, SIGTSTP);
				foreground->status = STOPPED;
			} 
			display_new();
            break;
		}
        
        case SIGCHLD: 
		{
            int status;
            pid_t pid;

            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                Job *current = find_job_by_pgid(pid);
				if (current && current->background) {
					if (WIFEXITED(status) || WIFSIGNALED(status)) {
						completed_jobs[completed_job_count].jid = current->jid;
						completed_jobs[completed_job_count++].command = strdup(current->jobstring);

						// remove job and set up shell for new line
						remove_job(pid);
					}
				}
			}
			break;
		}
    }
}

void setup_signal_handlers() {
    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);
    signal(SIGCHLD, sig_handler);
	signal(SIGTTOU, SIG_IGN);
}

int main() {
	setup_signal_handlers();

    while (1) {
        char *command = readline("# "); 

		// exit shell if Ctrl+D
        if (command == NULL) {
            break;
        } 

		print_completed_jobs();

		if (strcmp(command, "jobs") == 0) {
			list_jobs();
			continue;
		} else if (strcmp(command, "fg") == 0) {
			continue;
		} else if (strcmp(command, "bg") == 0) {
			continue;
		}

		Job *curr_job = parse_input(command);
		execute_job(curr_job);
		
		free(command);
    }
    
    return 0;
}