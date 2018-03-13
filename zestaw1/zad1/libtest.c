#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "mylib.h"

char* get_random_string(int length)
{
    char* string = calloc(length, sizeof(char));
    for(int i = 0; i < length; i++)
        string[i] = (char)(rand() % 95 + 33); // all ascii codes of non-white characters

    return string;
}

double subtract_time(struct timeval a, struct timeval b)
{
    double tmp_a = ((double) a.tv_sec) + ((double) a.tv_usec) / 1000000;
    double tmp_b = ((double) b.tv_sec) + ((double) b.tv_usec) / 1000000;
    return tmp_a - tmp_b;
}

void print_time(double d)
{
    int minutes = ((int) d) / 60;
    double seconds = d - ((double) minutes);
    printf("%dm%.3fs\n", minutes, seconds);
}

void static_test(int array_length, int block_length)
{
    printf("testing static structure...\n");

    static_array array;
    init_static_array(&array, array_length);

    char* buffer = calloc(20, sizeof(char));
    int index;

    while(strcmp(buffer, "exit") != 0)
    {
        struct rusage ru;
        struct timeval real_start, real_end, sys_start, sys_end, user_start, user_end;

        scanf("%s", buffer);
        if(strcmp(buffer, "add") == 0)
        {
            scanf("%d", &index);
            char* str = get_random_string(block_length);

            gettimeofday(&real_start, NULL);
            getrusage(RUSAGE_SELF, &ru);
            static_add_block(&array, str, block_length, index);
        }

        else if(strcmp(buffer, "delete") == 0)
        {
            scanf("%d", &index);

            gettimeofday(&real_start, NULL);
            getrusage(RUSAGE_SELF, &ru);
            static_delete_block(&array, index);
        }

        else if(strcmp(buffer, "find") == 0)
        {
            scanf("%d", &index);

            gettimeofday(&real_start, NULL);
            getrusage(RUSAGE_SELF, &ru);
            static_find_nearest_block(&array, index);
            //int result = static_find_nearest_block(&array, index);
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

        sys_start = ru.ru_stime;
        user_start = ru.ru_utime;

        gettimeofday(&real_end, NULL);
        getrusage(RUSAGE_SELF, &ru);
        sys_end = ru.ru_stime;
        user_end = ru.ru_utime;

        printf("%s execution time:\n", buffer);
        printf("real\t");
        print_time(subtract_time(real_end, real_start));
        printf("user\t");
        print_time(subtract_time(user_end, user_start));
        printf("sys\t");
        print_time(subtract_time(sys_end, sys_start));
    }
}

void dynamic_test(int array_length, int block_length)
{
    printf("testing dynamic structure...\n");

    dynamic_array array;
    init_dynamic_array(&array, array_length);

    char* buffer = calloc(20, sizeof(char));
    int index;

    while(strcmp(buffer, "exit") != 0)
    {
        struct rusage ru_start, ru_end;
        struct timeval real_start, real_end, sys_start, sys_end, user_start, user_end;

        scanf("%s", buffer);
        if(strcmp(buffer, "add") == 0)
        {
            scanf("%d", &index);
            char* str = get_random_string(block_length);

            gettimeofday(&real_start, NULL);
            getrusage(RUSAGE_SELF, &ru_start);
            dynamic_add_block(&array, str, block_length, index);
            gettimeofday(&real_end, NULL);
            getrusage(RUSAGE_SELF, &ru_end);
        }

        else if(strcmp(buffer, "delete") == 0)
        {
            scanf("%d", &index);

            gettimeofday(&real_start, NULL);
            getrusage(RUSAGE_SELF, &ru_start);
            dynamic_delete_block(&array, index);
            gettimeofday(&real_end, NULL);
            getrusage(RUSAGE_SELF, &ru_end);
        }

        else if(strcmp(buffer, "find") == 0)
        {
            scanf("%d", &index);

            gettimeofday(&real_start, NULL);
            getrusage(RUSAGE_SELF, &ru_start);
            dynamic_find_nearest_block(&array, index);
            gettimeofday(&real_end, NULL);
            getrusage(RUSAGE_SELF, &ru_end);
            //int result = dynamic_find_nearest_block(&array, index);
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

        sys_start = ru_start.ru_stime;
        user_start = ru_start.ru_utime;
        sys_end = ru_end.ru_stime;
        user_end = ru_end.ru_utime;

        printf("%s execution time:\n", buffer);
        printf("real\t");
        print_time(subtract_time(real_end, real_start));
        printf("user\t");
        print_time(subtract_time(user_end, user_start));
        printf("sys\t");
        print_time(subtract_time(sys_end, sys_start));
    }
}


int main(int argc, char** argv)
{
    srand(time(NULL));
    if(argc != 4)
    {
        fprintf(stderr, "wrong arguments number\n");
        exit(1);
    }

    int array_length = atoi(argv[2]);
    int block_length = atoi(argv[3]);

    if(strcmp(argv[1], "static") == 0)
        static_test(array_length, block_length);

    else if(strcmp(argv[1], "dynamic") == 0)
        dynamic_test(array_length, block_length);

    else
    {
        fprintf(stderr, "first argument should either be 'static' or 'dynamic'\n");
        exit(1);
    }
}
