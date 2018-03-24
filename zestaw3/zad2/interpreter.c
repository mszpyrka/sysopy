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

#define ARGS_LIMIT 15
int line_counter = 0; // Stores number of currently processed line in batch file

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

// Extracts program name from path (e.g. ./sysopy/zestaw1/zad1/main -> main)
char* extract_program_name(char* path) {

    int end_pos = 0;
    while(path[end_pos] != '\0')
        end_pos++;

    int start_pos = end_pos;
    while(start_pos > 0 && path[start_pos - 1] != '/')
        start_pos--;

    return path + start_pos;
}

// Parses single line from batch file
void process_next_line(char* buffer) {

    prepare_next_line(buffer);

    // Looks for the first word in the line
    char* command = strtok(buffer, " ");

    // If there are no words - returns (empty line)
    if(command == NULL)
        return;

    char* args[ARGS_LIMIT + 1];
    args[0] = extract_program_name(command);
    int arg_iter = 1;

    while((args[arg_iter] = strtok(NULL, " ")) != NULL) {
        arg_iter++;

        if(arg_iter > ARGS_LIMIT) {
            fprintf(stderr, "arguments number limit exceeded\n");
            exit(1);
        }
    }

    int process_id = fork();

    if(process_id < 0) {
        perror("Error occurred while using fork()");
        exit(1);
    }

    // Child process - executes command
    if(process_id == 0) {

        if(execvp(command, args) == -1) {

            fprintf(stderr, "Failed to run command '%s' in line %d: ", command, line_counter);
            perror("");
            exit(1);
        }
    }

    // TODO - error checking for child process status
    int exec_status;
    wait(&exec_status);

    if(WIFSIGNALED(exec_status)) {

        fprintf(stderr, "Command '%s' in line %d: process terminated by signal: %d", command, line_counter, WTERMSIG(exec_status));
        exit(1);
    }
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

        line_counter++;
        process_next_line(buffer);
    }
}
