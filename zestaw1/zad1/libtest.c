#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include "mylib.h"

char* get_random_string(int length)
{
    char* string = calloc(length, sizeof(char));
    for(int i = 0; i < length; i++)
        string[i] = (char)(rand() % 95 + 33); // all ascii codes for non-white characters
        
    return string;
}

void print_static(static_array* array)
{
    for(int i = 0; i < array -> array_length; i++)
    {
        printf("%d:\t", i);
        for(int j = 0; j < array -> block_lengths[i]; j++)
            printf("%c", array -> blocks[i][j]);
        
        printf("\n");
    }
}

void print_dynamic(dynamic_array* array)
{
    for(int i = 0; i < array -> array_length; i++)
    {
        printf("%d:\t", i);
        for(int j = 0; j < array -> block_lengths[i]; j++)
            printf("%c", array -> blocks[i][j]);
        
        printf("\n");
    }
}

void static_test(int array_length, int block_length)
{
    printf("testing static structure...\n");
    
    static_array array = init_static_array(array_length);
    
    char* buffer = calloc(20, sizeof(char));
    int index;
    
    while(strcmp(buffer, "exit") != 0)
    {
        struct rusage ru;
        struct timeval sys_start, sys_end, user_start, user_end;
        
        getrusage(RUSAGE_SELF, &ru);
        sys_start = ru.ru_stime;
        user_start = ru.ru_utime;
        
        scanf("%s", buffer);
        if(strcmp(buffer, "add") == 0)
        {
            scanf("%d", &index);
            char* str = get_random_string(block_length);
            static_add_block(&array, str, block_length, index);
        }
        
        else if(strcmp(buffer, "delete") == 0)
        {
            scanf("%d", &index);
            static_delete_block(&array, index);
        }
        
        else if(strcmp(buffer, "find") == 0)
        {
            scanf("%d", &index);
            int result = static_find_nearest_block(&array, index);
            //printf("%d\n", result);
        }
        
        else if(strcmp(buffer, "exit") == 0)
        {
            //print_static(&array);
            exit(0);
        }
        
        else
        {
            fprintf(stderr, "unknown command: %s\n", buffer);
        }
        
        getrusage(RUSAGE_SELF, &ru);
        sys_end = ru.ru_stime;
        user_end = ru.ru_utime;
        
        printf("%s execution time:\n", buffer);
        printf("real\t%d\n", 0);
        printf("user\t%ld.%lds\n", user_end.tv_sec - user_start.tv_sec, user_end.tv_usec - user_start.tv_usec);
        printf("sys\t%ld.%lds\n", sys_end.tv_sec - sys_start.tv_sec, sys_end.tv_usec - sys_start.tv_usec);
    }
}

void dynamic_test(int array_length, int block_length)
{
    printf("testing dynamic structure...\n");
    
    dynamic_array array = init_dynamic_array(array_length);
    
    char* buffer = calloc(20, sizeof(char));
    int index;
    
    while(strcmp(buffer, "exit") != 0)
    {
        struct rusage ru;
        struct timeval sys_start, sys_end, user_start, user_end;
        
        getrusage(RUSAGE_SELF, &ru);
        sys_start = ru.ru_stime;
        user_start = ru.ru_utime;
        
        scanf("%s", buffer);
        if(strcmp(buffer, "add") == 0)
        {
            scanf("%d", &index);
            char* str = get_random_string(block_length);
            dynamic_add_block(&array, str, block_length, index);
        }
        
        else if(strcmp(buffer, "delete") == 0)
        {
            scanf("%d", &index);
            dynamic_delete_block(&array, index);
        }
        
        else if(strcmp(buffer, "find") == 0)
        {
            scanf("%d", &index);
            int result = dynamic_find_nearest_block(&array, index);
            //printf("%d\n", result);
        }
        
        else if(strcmp(buffer, "exit") == 0)
        {
            //print_dynamic(&array);
            exit(0);
        }
        
        else
        {
            fprintf(stderr, "unknown command: %s\n", buffer);
        }
        
        getrusage(RUSAGE_SELF, &ru);
        sys_end = ru.ru_stime;
        user_end = ru.ru_utime;
        
        printf("%s execution time:\n", buffer);
        printf("real\t%d\n", 0);
        printf("user\t%lu.%06u\n", user_end.tv_sec - user_start.tv_sec, user_end.tv_usec - user_start.tv_usec);
        printf("sys\t%lu.%06u\n", sys_end.tv_sec - sys_start.tv_sec, sys_end.tv_usec - sys_start.tv_usec);
    }
}


int main(int argc, char** argv)
{
    srand(time(NULL));
    if(argc < 4)
    {
        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }
    
    int array_length = atoi(argv[1]);
    int block_length = atoi(argv[2]);
    
    if(strcmp(argv[3], "static") == 0)
        static_test(array_length, block_length);
    
    else if(strcmp(argv[3], "dynamic") == 0)
        dynamic_test(array_length, block_length);
    
    else
    {
        fprintf(stderr, "third argument should either be 'static' or 'dynamic'\n");
        exit(1);
    }
}