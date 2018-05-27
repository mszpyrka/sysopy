#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "general.h"

void init_cyclic_buffer(struct cyclic_buffer *buf, int size) {

    buf -> max_size = size;
    buf -> current_size = 0;
    buf -> front = 0;
    buf -> back = 1;
}

int buffer_write(struct cyclic_buffer *buf, const char *string) {

    if(buf -> current_size >= buf -> max_size)
        return -1;

    int position = (buf -> back - 1 + buf -> max_size) % buf -> max_size;
    buf -> strings[position] = malloc(sizeof(char) * (strlen(string) + 1));
    strcpy(buf -> strings[position], string);
    buf -> back = position;
    buf -> current_size++;

    return position;
}

int buffer_read(struct cyclic_buffer *buf, char *string) {

    if(buf -> current_size == 0)
        return -1;

    int position = buf -> front;
    strcpy(string, buf -> strings[position]);

    free(buf -> strings[position]);
    buf -> front = (buf -> front - 1 + buf -> max_size) % buf -> max_size;
    buf -> current_size--;

    return position;
}

// Logs a message to stderr
void print_log(const char *message, int producer) {

    char buffer[128];
    if(producer == 1)
        sprintf(buffer, "[log - producer %ld] ", pthread_self());

    else
        sprintf(buffer, "[log - consument %ld] ", pthread_self());

    strcat(buffer, message);

    fprintf(stderr, "%s\n", buffer);
}

