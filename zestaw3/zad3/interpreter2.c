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
#include <inttypes.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ARGS_LIMIT 15
int line_counter = 0; // Stores number of currently processed line in batch file

// Extracts time interval from two timeval structures
double subtract_time(struct timeval a, struct timeval b)
{
    double tmp_a = ((double) a.tv_sec)  + (((double) a.tv_usec) / 1000000);
    double tmp_b = ((double) b.tv_sec)  + (((double) b.tv_usec) / 1000000);
    return tmp_a - tmp_b;
}

// Prints time value in proper format
void print_time(double t)
{
    int minutes = (int) (t / 60);
    double seconds = t - minutes * 60;
    printf("%dm%.4fs\n", minutes, seconds);
}

// Prints program usage
void print_usage() {

    printf("usage: ./main <path_to_script> <cpu_time_limit_in_s> <mem_limit_in_MB>\n");
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
void process_next_line(char* buffer, struct rlimit* cpu_limit, struct rlimit* mem_limit) {

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

    struct rusage ru_start, ru_end;

    if(getrusage(RUSAGE_CHILDREN, &ru_start) < 0) {

        perror("Error occurred while obtaining resource usage for children processes");
        exit(1);
    }

    int process_id = fork();

    if(process_id < 0) {
        perror("Error occurred while using fork()");
        exit(1);
    }

    // Child process - executes command
    if(process_id == 0) {

        setrlimit(RLIMIT_AS, mem_limit);
        setrlimit(RLIMIT_CPU, cpu_limit);

        if(execvp(command, args) == -1) {

            fprintf(stderr, "Failed to run command '%s' in line %d: ", command, line_counter);
            perror("");
            exit(1);
        }
    }

    int exec_status;
    wait(&exec_status);

    if(WIFEXITED(exec_status) && WEXITSTATUS(exec_status) != 0) {

        fprintf(stderr, "Command '%s' in line %d: process exited with status: %d\n", command, line_counter, WEXITSTATUS(exec_status));
        exit(1);
    }

    if(WIFSIGNALED(exec_status)) {

        fprintf(stderr, "Failed to run command '%s' in line %d: process terminated by signal: %d\n", command, line_counter, WTERMSIG(exec_status));
        exit(1);
    }

    if(getrusage(RUSAGE_CHILDREN, &ru_end) < 0) {

        perror("Error occurred while obtaining resource usage for children processes");
        exit(1);
    }

    struct timeval sys_start, sys_end, user_start, user_end;
    sys_start = ru_start.ru_stime;
    user_start = ru_start.ru_utime;
    sys_end = ru_end.ru_stime;
    user_end = ru_end.ru_utime;

    printf(ANSI_COLOR_GREEN);
    printf("Command '%s' in line %d - resource usage:\n", command, line_counter);
    printf("usr time:\t");
    print_time(subtract_time(user_end, user_start));
    printf("sys time:\t");
    print_time(subtract_time(sys_end, sys_start));
    printf("mem:\t");
    printf("\n");/// TODO - memory usage calculating (???)
    printf(ANSI_COLOR_RESET);

}

int main(int argc, char** argv) {

    if(argc != 4) {

        fprintf(stderr, "Wrong arguments format\n");
        print_usage();
        exit(1);
    }

    // Setting cpu time and memory limits
    struct rlimit cpu_limit, mem_limit;
    cpu_limit.rlim_max = atoi(argv[2]);
    cpu_limit.rlim_cur = cpu_limit.rlim_max;

    mem_limit.rlim_max = ((long long int) atoi(argv[3])) << 20; // argv[3] stores mem limit in megabytes
    mem_limit.rlim_cur = mem_limit.rlim_max;

    FILE* input_file = fopen(argv[1], "r");
    char* buffer = NULL;
    size_t buffer_size = 0;
    size_t bytes_read;

    while((bytes_read = getline(&buffer, &buffer_size, input_file)) != -1) {

        line_counter++;
        process_next_line(buffer, &cpu_limit, &mem_limit);
    }
}
