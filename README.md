Project 1 - ECE 461S

yash project

This is a custom shell similar to bash that includes several basic features:

File redirection:
  -Creates files if they don't exist for output redirection
  -Fails command if input redirection (a file) does not exist

Piping:
  -The left command will have stdout replaced with the input to a pipe
  -The right command will have stdin replaced with the output from the same pipe
  -Children within the same pipeline will be in one process group for the pipeline
  -Children within the same pipeline will be started and stopped simultaneously

Signals (SIGINT, SIGTSTP, SIGCHLD)
  -Ctrl-c quits current foreground process (if one exists) and not the shell
  -Ctrl-z sends SIGTSTP to the current foreground process

Job control
  -Background jobs using &
  -fg sends SIGCONT to the most recent background or stopped process, prints the process to stdout , and waits for completion
  -bg sends SIGCONT to the most recent stopped process, prints the process to stdout in the jobs format, and doesn't wait for completion (as if &)
  -jobs print the job control table similar to bash. 
  -Terminated background jobs will be printed after the newline character sent on stdin with a Done in place of the Stopped or Running.


