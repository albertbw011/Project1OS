#define _POSIX_C_SOURCE 200809L
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

// extern char *strdup(const char*);
// extern int kill(pid_t pid, int sig);

/*
Generate cleaner new line
*/
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

            while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
                Job *current = find_job_by_pgid(pid);
				if (current && current->background) {
					if (WIFEXITED(status) || WIFSIGNALED(status)) {
						completed_jobs[completed_job_count].jid = current->jid;
						completed_jobs[completed_job_count++].command = strdup(current->jobstring);

						// remove job and set up shell for new line
						remove_job(pid);
					} else if (WIFSTOPPED(status)) {
						current->status = STOPPED;
					}
				}
			}
			break;
		}
    }
}

// Setting up signal handlers using sigaction()
void setup_signal_handlers() {
    struct sigaction sa;

    // Handle SIGINT (Ctrl+C)
    sa.sa_handler = sig_handler;
    sigaction(SIGINT, &sa, NULL);

    // Handle SIGTSTP (Ctrl+Z)
    sigaction(SIGTSTP, &sa, NULL);

    // Handle SIGCHLD
    sigaction(SIGCHLD, &sa, NULL);

    // Ignore SIGTTOU (background processes trying to write to the terminal)
    signal(SIGTTOU, SIG_IGN);
}

int main() {
	setup_signal_handlers();

    while (1) {
        char *command = readline("# "); 

		// exit shell if Ctrl+D
        if (command == NULL) {
			printf("\n");
            break;
        } 

		print_completed_jobs();

		if (strcmp(command, "jobs") == 0) {
			list_jobs();
		} else if (strcmp(command, "fg") == 0) {
			handle_fg();
		} else if (strcmp(command, "bg") == 0) {
			handle_bg();
		} else {
			Job *curr_job = parse_input(command);
			execute_job(curr_job);
		}

		free(command);
    }
    
    return 0;
}