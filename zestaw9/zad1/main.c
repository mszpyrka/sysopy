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
#include <pthread.h>
#include "general.h"

pthread_cond_t cond_buffer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mut_file = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_buffer = PTHREAD_MUTEX_INITIALIZER;

int filled_slots;
int empty_slots;

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

char *text_buffer;
#define TEXT_BUFFER_SIZE 256


// Reads all settings from settings file and opens input file.
void read_settings(const char *settings_file) {

    FILE *settings_fp = fopen(settings_file, "r");

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

// Reads next line from input file
int read_next_line(char *buffer) {

    pthread_mutex_lock(&mut_file);

    size_t size = TEXT_BUFFER_SIZE;
    if(getline(&text_buffer, &size, input_file) == -1) {
        pthread_mutex_unlock(&mut_file);
        return -1;
    }
    
    strcpy(buffer, text_buffer);
    buffer[strlen(buffer) - 1] = (char)0;
    pthread_mutex_unlock(&mut_file);

    return 0;
}

// Creates and initializes all necessery IPC structures
void initialize_memory() {

    init_cyclic_buffer(&products_buffer, buffer_size);

    empty_slots = buffer_size;
    filled_slots = 0;
}


// Function executed by producer thread to insert single element into buffer
int produce() {
    char line[TEXT_BUFFER_SIZE];

    pthread_mutex_lock(&mut_buffer);

    while(empty_slots == 0)
        pthread_cond_wait(&cond_buffer, &mut_buffer);

    if(read_next_line(line) == -1) {
        filled_slots++;
        pthread_cond_broadcast(&cond_buffer);
        pthread_mutex_unlock(&mut_buffer);
        return -1;
    }
    int position = buffer_write(&products_buffer, line);

    filled_slots++;
    empty_slots--;
    pthread_cond_broadcast(&cond_buffer);
    pthread_mutex_unlock(&mut_buffer);
    return position;
}


// Checks if given string length satisfies the condition of strings filtering
int satisfies_condition(int length) {
    if(search_mode == -1 && length < compare_constant)
        return 1;

    if(search_mode == 0 && length == compare_constant)
        return 1;

    if(search_mode == 1 && length > compare_constant)
        return 1;

    return 0;
}


// Function executed by producer thread to insert single element into buffer
int consume() {
    char line[TEXT_BUFFER_SIZE];

    pthread_mutex_lock(&mut_buffer);

    while(filled_slots == 0)
        pthread_cond_wait(&cond_buffer, &mut_buffer);

    int position = buffer_read(&products_buffer, line);

    if(position == -1) {
        filled_slots++;
        pthread_cond_broadcast(&cond_buffer);
        pthread_mutex_unlock(&mut_buffer);
        return -1;
    }
    
    if(satisfies_condition(strlen(line)) == 1) {
        char result[TEXT_BUFFER_SIZE + 20];
        sprintf(result, "%d: \"%s\"", position, line);
        print_log(result, 0);
    }

    filled_slots--;
    empty_slots++;
    pthread_cond_broadcast(&cond_buffer);
    pthread_mutex_unlock(&mut_buffer);
    return 0;
}


void *producer_thread_function(void *arg) {

    while(1) {

        if(produce() == -1) {
       
            return (void*)0;
        }
    }
}

void *consumer_thread_function(void *arg) {

    while(1) {

        if(consume() == -1){
         
            return (void*)0;
        }
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

    text_buffer = malloc(sizeof(char) * TEXT_BUFFER_SIZE);

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

    pthread_t threads[producers_number + consumers_number];

    for(int i = 0; i < producers_number; i++)
        pthread_create(threads + i, NULL, producer_thread_function, NULL);

    for(int i = producers_number; i < producers_number + consumers_number; i++)
        pthread_create(threads + i, NULL, consumer_thread_function, NULL);

    for(int i = 0; i < producers_number + consumers_number; i++)
        pthread_join (threads[i], NULL);

}
