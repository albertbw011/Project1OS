Project 1 - ECE 461S

yash project

This is a custom shell similar to bash that includes several basic features:

File redirection:
  with creation of files if they don't exist for output redirection
  fail command if input redirection (a file) does not exist

Piping
  The left command will have stdout replaced with the input to a pipe
  The right command will have stdin replaced with the output from the same pipe
  Children within the same pipeline will be in one process group for the pipeline
  Children within the same pipeline will be started and stopped simultaneously

Signals (SIGINT, SIGTSTP, SIGCHLD)
  Ctrl-c must quit current foreground process (if one exists) and not the shell and should not print the process (unlike bash)
  Ctrl-z must send SIGTSTP to the current foreground process and should not print the process (unlike bash)
  The shell will not be stopped on SIGTSTP

Job control
  Background jobs using &
  fg must send SIGCONT to the most recent background or stopped process, print the process to stdout , and wait for completion
  bg must send SIGCONT to the most recent stopped process, print the process to stdout in the jobs format, and not wait for completion (as if &)
  jobs will print the job control table similar to bash. 
  Terminated background jobs will be printed after the newline character sent on stdin with a Done in place of the Stopped or Running.


