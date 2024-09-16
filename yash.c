#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <signal.h>
#include <sys/wait.h>

#include "parsing.h"
#include "jobs.h"

/*
Handle signals
*/
void sig_handler(int signo) {
    switch (signo) {
        case SIGINT:
            if (pid1 > 0) {
                kill(pid1, SIGINT);
            } 
			if (pid2 > 0) {
				kill(pid2, SIGINT);
			}

			if (pid1 > 0 || pid2 > 0) {
				printf("\n");
			}

			if (pid1 == -1 && pid2 == -1) {
				printf("\n");
				rl_replace_line("", 0);
				rl_on_new_line();
				rl_redisplay();
			}
            break;

        case SIGSTOP:
            if (pid1 > 0) {
                kill(pid1, SIGTSTP);
            }
			if (pid2 > 0) {
                kill(pid2, SIGTSTP);
            }
            break;
        
        case SIGCHLD: 
		{
            int status;
            pid_t pid;

            while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
                if (pid == pid1) {
                    pid1 = -1;
                }
				if (pid == pid2) {
					pid2 = -1;
				}
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
    char *command;

    while (1) {
		setup_signal_handlers();
        command = readline("# "); 

		// exit shell if Ctrl+D
        if (command == NULL) {
            break;
        } 

		Job *curr_job = parse_input(command);
		execute_job(curr_job);
    }
    
    return 0;
}