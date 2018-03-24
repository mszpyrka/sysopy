#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define ARGS_LIMIT 20

// Prints program usage
void print_usage() {

    printf("usage: ./main <path_to_script>\n");
}

// Changes all white spaces in buffer into 'space' characters (for easier tokenization purposes)
void prepare_next_line(char* buffer) {

    for(int i = 0; buffer[i] != '\0'; i++)
        if(isspace(buffer[i]))
            buffer[i] = ' ';
}

// Parses single line from batch file
void parse_next_line(char* buffer) {

    prepare_next_line(buffer);

    // Looks for the first word in the line
    char* prog_name = strtok(buffer, " ");

    // If there are no words - returns (empty line)
    if(prog_name == NULL)
        return;

    char* args[ARGS_LIMIT + 1];
    int arg_iter = 0;

    while((args[arg_iter] = strtok(NULL, " ")) != NULL) {
        arg_iter++;

        if(arg_iter > ARGS_LIMIT) {
            fprintf(stderr, "arguments number limit exceeded\n", );
            exit(1);
        }
        printf("%s,", next_arg);
    }

    printf("\n");
}

int main(int argc, char** argv) {

    if(argc != 2) {

        fprintf(stderr, "Wrong arguments format\n");
        print_usage();
        exit(1);
    }

    FILE* input_file = fopen(argv[1], "r");
    char* buffer = NULL;
    size_t buffer_size = 0;
    size_t bytes_read;

    while((bytes_read = getline(&buffer, &buffer_size, input_file)) != -1) {

        parse_next_line(buffer);
    }
}
