#ifndef MYLIB_H
#define MYLIB_H

#define STATIC_ARRAY_MAX_SIZE 100
#define STATIC_CHAR_BLOCK_MAX_SIZE 1000000

typedef struct static_array
{
    int array_length;
    char blocks[STATIC_ARRAY_MAX_SIZE][STATIC_CHAR_BLOCK_MAX_SIZE];
    int block_lengths[STATIC_ARRAY_MAX_SIZE];
} static_array;

typedef struct dynamic_array
{
    int array_length;
    char** blocks;
    int* block_lengths;
} dynamic_array;


static_array init_static_array(int array_length);
int static_find_nearest_block(static_array* array, int index);
void static_add_block(static_array* array, const char* values, int length, int index);
void static_delete_block(static_array* array, int index);

dynamic_array init_dynamic_array(int array_length);
int dynamic_find_nearest_block(dynamic_array* array, int index);
void dynamic_add_block(dynamic_array* array, const char* values, int length, int index);
void dynamic_delete_block(dynamic_array* array, int index);

#endif
