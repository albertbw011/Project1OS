#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>

typedef struct { 
    char *command;
    char *args;
    char *input_file;
    char *output_file;
    int append_output;  
} process;

void parse_input(char *input);

int main() {

    while (1) {
        printf("yash> ");
        fflush(stdout);
    }
    return 0;
}