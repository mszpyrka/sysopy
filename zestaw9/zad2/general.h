#ifndef TASK9_GENERAL_H
#define TASK9_GENERAL_H

#define CYCLIC_BUFFER_MAX_SIZE  128

#define TEXT_BUFFER_SIZE 1024


struct cyclic_buffer {

    int max_size;
    int current_size;
    char *strings[CYCLIC_BUFFER_MAX_SIZE];
    int front;
    int back;
};


int buffer_write(struct cyclic_buffer *buf, const char *string);
int buffer_read(struct cyclic_buffer *buf, char *string);
void init_cyclic_buffer(struct cyclic_buffer *buf, int size);

void print_log(const char *message, int producer);

#endif
