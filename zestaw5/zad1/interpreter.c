#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

#define ARGS_LIMIT 15
#define PIPELINE_MAX_SIZE 20
int line_counter = 0; // Stores number of currently processed line in batch file
pid_t *child_pid_array;
int child_pid_iter;

// Function used when exiting from program - kills all children that are still running
static void exit_fun() {

    for(int i = 0; i < child_pid_iter; i++) {

        if(waitpid(child_pid_array[i], NULL, WNOHANG) == 0)
            kill(child_pid_array[i], SIGINT);
    }
}

// Executes given shell command, redirects input_fd into STDIN, return file descriptor for process STDOUT
int pipe_exec(const char *command, int input_fd, int *child_pid, int WRITE_STDOUT) {

    int pipe_fd[2];

    if((WRITE_STDOUT == 0) && (pipe(pipe_fd) == -1)) {
        perror("Error occurred while using pipe()");
        exit(1);
    }

    if((*child_pid = fork()) < 0)  {
        perror("Error occurred while using fork()");
        exit(1);
    }

    if(*child_pid == 0) {

        if(input_fd != STDIN_FILENO) {

            //fprintf(stdout, "XDD\n");
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        if((WRITE_STDOUT == 0) && (pipe_fd[1] != STDOUT_FILENO)) {

            //fprintf(stdout, "XDD\n");
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);
        }

        execl("/bin/sh", "sh", "-c", command, NULL);
        exit(2);
    }

    if(WRITE_STDOUT == 0) {
        close(pipe_fd[1]);
        return pipe_fd[0];
    }

    return STDOUT_FILENO;
}

int is_empty(const char* line) {

    int i = 0;
    while(line[i] != '\0') {

        if(isspace(line[i]) != 0)
            return 0;

        i++;
    }

    return 1;
}

void process_next_line(char *command) {

    if(is_empty(command) == 1)
        return;

    int *command_pointers = malloc(sizeof(int) * PIPELINE_MAX_SIZE);
    int command_iter = 1;
    command_pointers[0] = 0;

    // Puts string termination character into each position of pipe char
    int i = 0;
    while(command[i] != '\0') {

        if(command[i] == '|') {

            command[i] = '\0';
            command_pointers[command_iter] = i+1;

            if(is_empty(command + command_pointers[command_iter-1]) == 1) {

                fprintf(stderr, RED "Error: line %d - incorrect syntax\n" RESET, line_counter);
                exit(1);
            }

            command_iter++;
        }

        i++;
    }

    child_pid_iter = 0;
    int child_pid;
    int input_fd = STDIN_FILENO;
    int stdout_flag = 0;

    for(int i = 0; i < command_iter; i++) {

        if(i == command_iter - 1)
            stdout_flag = 1;

        input_fd = pipe_exec(command + command_pointers[i], input_fd, &child_pid, stdout_flag);
        child_pid_array[child_pid_iter] = child_pid;
        child_pid_iter++;
    }

    int status;
    for(int i = 0; i < child_pid_iter; i++) {

        waitpid(child_pid_array[i], &status, 0);

        if(WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0)
            continue;

        else if(WIFSIGNALED(status) != 0) {

            fprintf(stderr, RED "Error: command %s in line %d - process terminated by signal %d\n" RESET, command + command_pointers[i], line_counter, WTERMSIG(status));
            exit(1);
        }
        else {

            fprintf(stderr, RED "Error: command %s in line %d - process exited with status %d\n" RESET, command + command_pointers[i], line_counter, WEXITSTATUS(status));
            exit(1);
        }
    }

    child_pid_iter = 0;
}

// Prints program usage
void print_usage() {

    printf("usage: ./main <path_to_script>\n");
}

int main(int argc, char** argv) {

    fflush(stdout);

    if (atexit(exit_fun) != 0) {

        perror("Cannot set atexit function");
        exit(1);
    }

    child_pid_array = malloc(PIPELINE_MAX_SIZE * sizeof(pid_t));

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
