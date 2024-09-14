#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <signal.h>

#include "parsing.c"
#include "jobs.c"

pid_t pid1 = -1;
pid_t pid2 = -1;


int handle_redirection(Command *cmd);

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

/*
Executes commands with execvp()
*/
void execute_command(Command *cmd) {
    pid1 = fork();
    int status;

    if (pid1 < 0) {
        perror("fork error");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        if (handle_redirection(cmd) == 0) {
            if (execvp(cmd->command, cmd->args) == -1)
                exit(EXIT_FAILURE);
        } 
    } else {
		setpgid(pid1, pid1);
		add_job(pid1, *cmd, cmd->background ? BACKGROUND : RUNNING);

		if (!cmd->background) {
			int status;
			waitpid(pid1, &status, WUNTRACED);

			if (WIFSTOPPED(status)) {
				// WIP
			}
		}
        
		pid1 = -1;
    }
}

/*
Executes commands with pipes
Takes in two Command structs as inputs
*/
void execute_pipe(Command *left_command, Command *right_command) {
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
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        if (execvp(left_command->command, left_command->args) == -1) {
            fprintf(stderr, "%s: command not found\n", left_command->command);
            exit(EXIT_FAILURE);
        }
    } 

    pid2 = fork();
    if (pid2 < 0) {
        perror("fork error");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        if (execvp(right_command->command, right_command->args) == -1) {
            fprintf(stderr, "%s: command not found\n", right_command->command);
            exit(EXIT_FAILURE);
        }
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

	pid1 = -1;
	pid2 = -1;
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


int main() {
    char *command;

    while (1) {
		setup_signal_handlers();
        command = readline("# "); 

		// exit shell if Ctrl+D
        if (command == NULL) {
            break;
        } 

		// check for pipe
        char *pipe_pos = strstr(command, "|");
        
		// if there is a pipe, we parse each side into its own Command struct
		// otherwise we execute the command as is
        if (pipe_pos != NULL) {
            *pipe_pos = '\0';
            char *left_command = command;
            char *right_command = pipe_pos + 1;

            // allocate array for each command in the pipe
            Command **commands = (Command **) malloc(2 * sizeof(Command *));
            commands[0] = parse_input(left_command);
            commands[1] = parse_input(right_command);
            execute_pipe(commands[0], commands[1]);
            free_command(commands[0]);
            free_command(commands[1]);
            free(commands);
        } else {
            Command *cmd = parse_input(command);
            execute_command(cmd);
            free_command(cmd);
			free(cmd);
        }
    }
    
    return 0;
}