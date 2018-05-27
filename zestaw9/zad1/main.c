#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "general.h"

sem_t *sem_buffer; 
sem_t *sem_empty_slots;
sem_t *sem_filled_slots;
struct cyclic_buffer products_buffer;

int producers_number;
int consumers_number;
int buffer_size;
FILE *input_file;
int search_mode;
int compare_constant;
int search_mode;
int logging_mode;
int term_condition;

// Reads all settings from settings file and opens input file.
void read_settings(const char *settings_file) {

    FILE *settings_fp = fopen(settings_file, "r");
    perror("file opening");

    fscanf(settings_fp, "%d", &producers_number);

    fscanf(settings_fp, "%d", &consumers_number);
    fscanf(settings_fp, "%d", &buffer_size);

    char tmp_buf[32];
    fscanf(settings_fp, "%s", tmp_buf);
    input_file = fopen(tmp_buf, "r");

    fscanf(settings_fp, "%d", &compare_constant);

    fscanf(settings_fp, "%s", tmp_buf);
    if(tmp_buf[0] == '<')
        search_mode = -1;

    else if(tmp_buf[0] == '>')
        search_mode = 1;

    else if(tmp_buf[0] == '=')
        search_mode = 0;

    else {
        fprintf(stderr, "search mode should be one of [ < | = | > ]\n");
        exit(1);
    }

    fscanf(settings_fp, "%s", tmp_buf);
    if(strcmp(tmp_buf, "simple") == 0)
        logging_mode = 0;

    else if(strcmp(tmp_buf, "verbose") == 0)
        logging_mode = 1;

    else {
        fprintf(stderr, "search mode should be one of [ simple | verbose ]\n");
        exit(1);
    }

    fscanf(settings_fp, "%d", &term_condition);
}

// Creates and initializes all necessery IPC structures
void initialize_memory() {

    init_cyclic_buffer(&products_buffer, buffer_size);

    sem_buffer = malloc(sizeof(sem_t));
    sem_empty_slots = malloc(sizeof(sem_t));
    sem_filled_slots = malloc(sizeof(sem_t));

    if(sem_init(sem_buffer, 0, 1) != 0) {
        perror("Error occurred while creating buffer semaphore");
        exit(1);
    }

    if(sem_init(sem_empty_slots, 0, buffer_size) != 0) {
        perror("Error occurred while creating empty slots semaphore");
        exit(1);
    }

    if(sem_init(sem_filled_slots, 0, 0) != 0) {
        perror("Error occurred while creating filled slots semaphore");
        exit(1);
    }
}


// Function used when exiting from program - deletes all created IPC structures
static void exit_fun() {

    if(sem_buffer != (sem_t*)-1) {

        if(sem_destroy(sem_buffer))
            perror("Error occurred while deleting buffer semaphore");
    }

    if(sem_empty_slots != (sem_t*)-1) {

        if(sem_destroy(sem_empty_slots))
            perror("Error occurred while deleting empty slots semaphore");
    }

    if(sem_filled_slots != (sem_t*)-1) {

        if(sem_destroy(sem_filled_slots))
            perror("Error occurred while deleting filled slots semaphore");
    }
}



// Function used for handling SIGINT
void sig_int(int signo) {

    fprintf(stderr, "SIGINT received\n");
    exit(1);
}


void print_usage() {

    printf("Example usage: ./main <spec_file>\n");
}

int main(int argc, char** argv) {

    sem_buffer = (sem_t*)-1;
    sem_empty_slots = (sem_t*)-1;
    sem_filled_slots = (sem_t*)-1;

    if (atexit(exit_fun) != 0) {

        perror("Cannot set atexit function");
        exit(1);
    }

    // struct sigaction setup
    struct sigaction act_int;
    act_int.sa_handler = sig_int;
    sigemptyset(&act_int.sa_mask);
    act_int.sa_flags = 0;

    // Setting handler for SIGINT
    if(sigaction(SIGINT, &act_int, NULL) == -1) {

        perror("Cannot set SIGINT signal handler");
        exit(1);
    }

    
    if(argc != 2) {
        printf("Wrong arguments format\n");
        print_usage();
        exit(1);
    }

    read_settings(argv[1]);
    initialize_memory();

    printf("%d, %d, %d, %d, %d, %d, %d\n", consumers_number, producers_number, buffer_size,
        compare_constant, search_mode, logging_mode, term_condition);

    fflush(stdout);

    while(1) {
        pause();
    }
}
