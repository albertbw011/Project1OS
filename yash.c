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

/*
Handle signals
*/
void sig_handler(int signo) {
    switch (signo) {
		// Ctrl+C
        case SIGINT:
		{
            Job *foreground = get_foreground_job();
			printf("\n");
			if (foreground != NULL) {
				kill(-foreground->pgid, SIGINT);
			} else {
				rl_replace_line("", 0);
				rl_on_new_line();
				rl_redisplay();
			}
            break;
		}
		
		// Ctrl+Z
        case SIGSTOP:
		{
            Job *foreground = get_foreground_job();
			if (foreground != NULL) {
				kill(-foreground->pgid, SIGSTOP);
			}
            break;
		}
        
        case SIGCHLD: 
		{
            int status;
            pid_t pid;

            while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
                Job *current = find_job_by_pgid(pid);
				remove_job(pid);
				printf("[%d] - Done\t\t", current->jid);
				print_command(current->command);
				printf("\n");
			}
		}
            break;
    }
}

void setup_signal_handlers() {
    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);
    signal(SIGCHLD, sig_handler);
}

int main() {
    while (1) {
		setup_signal_handlers();
        char *command = readline("# "); 

		// exit shell if Ctrl+D
        if (command == NULL) {
            break;
        } 

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