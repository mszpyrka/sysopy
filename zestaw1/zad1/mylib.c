#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mylib.h"


// Utility functions
int get_block_sum(char* block, int length)
{
    int sum = 0;
    for(int i = 0; i < length; i++)
        sum += (int) block[i];

    return sum;
}


// Static structure functions
static_array init_static_array(int array_length)
{
    static_array array;
    array.array_length = array_length;
    
    for(int i = 0; i < array_length; i++)
        array.block_lengths[i] = 0;
    
    return array;
}


int static_find_nearest_block(static_array* array, int index)
{
    int target_value = get_block_sum(array -> blocks[index], array -> block_lengths[index]);
    
    int result = -1;
    int min_difference = -1;
    
    for(int i = 0; i < array -> array_length; i++)
    {
        if(i == index)
            continue;
        
        int tmp_value = get_block_sum(array -> blocks[i], array -> block_lengths[i]);
        
        if(min_difference == -1 || (tmp_value - target_value) < min_difference)
        {
            min_difference = abs(tmp_value - target_value);
            result = i;
        }
    }
    
    return result;
}


void static_add_block(static_array* array, const char* values, int length, int index)
{
    strcpy(array -> blocks[index], values);
    array -> block_lengths[index] = length;
}


void static_delete_block(static_array* array, int index)
{
    array -> block_lengths[index] = 0;
}


// Dynamic structure functions
dynamic_array init_dynamic_array(int array_length)
{
    dynamic_array array;
    array.array_length = array_length;
    array.blocks = calloc(array_length, sizeof(char*));
    array.block_lengths = calloc(array_length, sizeof(int));
    for(int i = 0; i < array_length; i++)
        array.block_lengths[i] = 0;
    
    return array;
}


void delete_dynamic_array(dynamic_array* array)
{
    for(int i = 0; i < array -> array_length; i++)
        free(array -> blocks[i]);
    
    free(array -> blocks);
}


int dynamic_find_nearest_block(dynamic_array* array, int index)
{
    int target_value = get_block_sum(array -> blocks[index], array -> block_lengths[index]);
    
    int result = -1;
    int min_difference = -1;
    
    for(int i = 0; i < array -> array_length; i++)
    {
        if(i == index)
            continue;
        
        int tmp_value = get_block_sum(array -> blocks[i], array -> block_lengths[i]);
        
        if(min_difference == -1 || (tmp_value - target_value) < min_difference)
        {
            min_difference = abs(tmp_value - target_value);
            result = i;
        }
    }
    
    return result;
}


void dynamic_add_block(dynamic_array* array, const char* values, int length, int index)
{
    array -> blocks[index] = realloc(array -> blocks[index], length * sizeof(char));
    strcpy(array -> blocks[index], values);
    array -> block_lengths[index] = length;
}


void dynamic_delete_block(dynamic_array* array, int index)
{
    free(array -> blocks[index]);
    array -> blocks[index] = NULL;
    array -> block_lengths[index] = 0;
}