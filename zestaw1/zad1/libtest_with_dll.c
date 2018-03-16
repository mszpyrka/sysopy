#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/time.h>
#include <dlfcn.h>
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
    double tmp_a = ((double) a.tv_sec)  + (((double) a.tv_usec) / 1000000);
    double tmp_b = ((double) b.tv_sec)  + (((double) b.tv_usec) / 1000000);
    return tmp_a - tmp_b;
}

void print_time(double t)
{
    int minutes = (int) (t / 60);
    double seconds = t - minutes * 60;
    printf("%dm%.4fs\n", minutes, seconds);
}

void static_t(int argc, char** argv)
{
    void* handle = dlopen("./libshared.so", RTLD_LAZY);
    if(!handle)
    {
        fprintf(stderr, "error occured while loading library:\n");
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    void (*d_init_static_array)(static_array*, int);
    d_init_static_array = dlsym(handle, "init_static_array");
    int (*d_static_find_nearest_block)(static_array*, int);
    d_static_find_nearest_block = dlsym(handle, "static_find_nearest_block");
    void (*d_static_add_block)(static_array*, const char*, int, int);
    d_static_add_block = dlsym(handle, "static_add_block");
    void (*d_static_delete_block)(static_array*, int);
    d_static_delete_block = dlsym(handle, "static_delete_block");

    static_array array;
    int array_length;
    int block_length;

    struct rusage ru_start, ru_end;
    struct timeval sys_start, sys_end, user_start, user_end;
    clock_t real_start, real_end;

    if(strcmp(argv[2], "create_table") == 0)
    {
        array_length = atoi(argv[3]);
        block_length = atoi(argv[4]);

        real_start = clock();

        getrusage(RUSAGE_SELF, &ru_start);
        d_init_static_array(&array, array_length);
        real_end = clock();

        getrusage(RUSAGE_SELF, &ru_end);

        sys_start = ru_start.ru_stime;
        user_start = ru_start.ru_utime;
        sys_end = ru_end.ru_stime;
        user_end = ru_end.ru_utime;

        printf("create_table execution time:\n");
        printf("real\t");
        print_time(((double) real_end - real_start) / CLOCKS_PER_SEC);
        printf("user\t");
        print_time(subtract_time(user_end, user_start));
        printf("sys\t");
        print_time(subtract_time(sys_end, sys_start));
    }

    else
    {
        fprintf(stderr, "cannot perform any operations on uninitialized table");
        exit(1);
    }

    int i = 5;
    while(i < argc)
    {
        char* buffer = argv[i];

        if(strcmp(argv[i], "search_element") == 0)
        {
            int number = atoi(argv[i+1]);

            real_start = clock();
            getrusage(RUSAGE_SELF, &ru_start);
            d_static_find_nearest_block(&array, number);
            real_end = clock();
            getrusage(RUSAGE_SELF, &ru_end);

            i += 2;
        }

        else if(strcmp(argv[i], "add") == 0)
        {
            int number = atoi(argv[i+1]);
            char* random_str = get_random_string(block_length);

            real_start = clock();
            getrusage(RUSAGE_SELF, &ru_start);

            while(number)
            {
                number--;
                d_static_add_block(&array, random_str, block_length, number);
            }

            real_end = clock();
            getrusage(RUSAGE_SELF, &ru_end);

            i += 2;
        }

        else if(strcmp(argv[i], "remove") == 0)
        {
            int number = atoi(argv[i+1]);

            real_start = clock();
            getrusage(RUSAGE_SELF, &ru_start);

            while(number)
            {
                number--;
                d_static_delete_block(&array, number);
            }

            real_end = clock();
            getrusage(RUSAGE_SELF, &ru_end);

            i += 2;
        }

        else if(strcmp(argv[i], "add_and_remove") == 0)
        {
            int number = atoi(argv[i+1]);
            char* random_str = get_random_string(block_length);

            real_start = clock();
            getrusage(RUSAGE_SELF, &ru_start);

            while(number >= 0)
            {
                number--;
                d_static_add_block(&array, random_str, block_length, 0);
                d_static_delete_block(&array, 0);
            }

            real_end = clock();
            getrusage(RUSAGE_SELF, &ru_end);

            i += 2;
        }

        else
        {
            fprintf(stderr, "unknown command: %s\n", argv[i]);
            exit(1);
        }

        sys_start = ru_start.ru_stime;
        user_start = ru_start.ru_utime;
        sys_end = ru_end.ru_stime;
        user_end = ru_end.ru_utime;

        printf("%s execution time:\n", buffer);
        printf("real\t");
        print_time(((double) real_end - real_start) / CLOCKS_PER_SEC);
        printf("user\t");
        print_time(subtract_time(user_end, user_start));
        printf("sys\t");
        print_time(subtract_time(sys_end, sys_start));

    }

    dlclose(handle);
}

void dynamic_t(int argc, char** argv)
{
    void* handle = dlopen("./libshared.so", RTLD_LAZY);
    if(!handle)
    {
        fprintf(stderr, "error occured while loading library:\n");
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    void (*d_init_dynamic_array)(dynamic_array*, int);
    d_init_dynamic_array = dlsym(handle, "init_dynamic_array");
    int (*d_dynamic_find_nearest_block)(dynamic_array*, int);
    d_dynamic_find_nearest_block = dlsym(handle, "dynamic_find_nearest_block");
    void (*d_dynamic_add_block)(dynamic_array*, const char*, int, int);
    d_dynamic_add_block = dlsym(handle, "dynamic_add_block");
    void (*d_dynamic_delete_block)(dynamic_array*, int);
    d_dynamic_delete_block = dlsym(handle, "dynamic_delete_block");

    dynamic_array array;
    int array_length;
    int block_length;

    struct rusage ru_start, ru_end;
    struct timeval sys_start, sys_end, user_start, user_end;
    clock_t real_start, real_end;

    if(strcmp(argv[2], "create_table") == 0)
    {
        array_length = atoi(argv[3]);
        block_length = atoi(argv[4]);

        real_start = clock();
        getrusage(RUSAGE_SELF, &ru_start);
        d_init_dynamic_array(&array, array_length);
        real_end = clock();
        getrusage(RUSAGE_SELF, &ru_end);

        sys_start = ru_start.ru_stime;
        user_start = ru_start.ru_utime;
        sys_end = ru_end.ru_stime;
        user_end = ru_end.ru_utime;

        printf("create_table execution time:\n");
        printf("real\t");
        print_time(((double) real_end - real_start) / CLOCKS_PER_SEC);
        printf("user\t");
        print_time(subtract_time(user_end, user_start));
        printf("sys\t");
        print_time(subtract_time(sys_end, sys_start));
    }

    else
    {
        fprintf(stderr, "cannot perform any operations on uninitialized table");
        exit(1);
    }

    int i = 5;
    while(i < argc)
    {
        char* buffer = argv[i];

        if(strcmp(argv[i], "search_element") == 0)
        {
            int number = atoi(argv[i+1]);

            real_start = clock();
            getrusage(RUSAGE_SELF, &ru_start);
            d_dynamic_find_nearest_block(&array, number);
            real_end = clock();
            getrusage(RUSAGE_SELF, &ru_end);

            i += 2;
        }

        else if(strcmp(argv[i], "add") == 0)
        {
            int number = atoi(argv[i+1]);
            char* random_str = get_random_string(block_length);

            real_start = clock();
            getrusage(RUSAGE_SELF, &ru_start);

            while(number)
            {
                number--;
                d_dynamic_add_block(&array, random_str, block_length, number);
            }

            real_end = clock();
            getrusage(RUSAGE_SELF, &ru_end);

            i += 2;
        }

        else if(strcmp(argv[i], "remove") == 0)
        {
            int number = atoi(argv[i+1]);

            real_start = clock();
            getrusage(RUSAGE_SELF, &ru_start);

            while(number)
            {
                number--;
                d_dynamic_delete_block(&array, number);
            }

            real_end = clock();
            getrusage(RUSAGE_SELF, &ru_end);

            i += 2;
        }

        else if(strcmp(argv[i], "add_and_remove") == 0)
        {
            int number = atoi(argv[i+1]);
            char* random_str = get_random_string(block_length);

            real_start = clock();
            getrusage(RUSAGE_SELF, &ru_start);

            while(number >= 0)
            {
                number--;
                d_dynamic_add_block(&array, random_str, block_length, 0);
                d_dynamic_delete_block(&array, 0);
            }

            real_end = clock();
            getrusage(RUSAGE_SELF, &ru_end);

            i += 2;
        }

        else
        {
            fprintf(stderr, "unknown command: %s\n", argv[i]);
            exit(1);
        }

        sys_start = ru_start.ru_stime;
        user_start = ru_start.ru_utime;
        sys_end = ru_end.ru_stime;
        user_end = ru_end.ru_utime;

        printf("%s execution time:\n", buffer);
        printf("real\t");
        print_time(((double) real_end - real_start) / CLOCKS_PER_SEC);
        printf("user\t");
        print_time(subtract_time(user_end, user_start));
        printf("sys\t");
        print_time(subtract_time(sys_end, sys_start));

    }

    dlclose(handle);
}

int main(int argc, char** argv)
{
    srand(time(NULL));

    if(argc < 2)
    {
        fprintf(stderr, "wrong input format\n");
        exit(1);
    }

    if(strcmp(argv[1], "static") == 0)
        static_t(argc, argv);

    else if(strcmp(argv[1], "dynamic") == 0)
        dynamic_t(argc, argv);

    else
    {
        fprintf(stderr, "first argument should either be 'static' or 'dynamic'\n");
        exit(1);
    }
}
