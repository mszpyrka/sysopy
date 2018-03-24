#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// Fills array with randomly generated values (in this case -> char values)
void generate_random_record(char* buffer, int bytes_number) {

    for(int i = 0; i < bytes_number; i++)
        buffer[i] = (char)(rand() % ('z' - 'a' + 1) + 'a'); // all ascii codes of non-white characters
}

// Creates file with randomly generated records
void generate(int argc, char** argv) {

    if(argc < 5) {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }

    // Parsing all arguments
    char* filename = argv[2];
    int records_number = atoi(argv[3]);
    int records_length = atoi(argv[4]);

    // Opening file
    FILE* file = fopen(filename, "w");

    if(file == NULL) {

        fprintf(stderr, "error occurred while opening file\n");
        exit(1);
    }

    char* buffer = malloc(records_length * sizeof(char));

    // Generating random records and writing them into file
    for(int i = 0; i < records_number; i++) {

        generate_random_record(buffer, records_length);

        if(fwrite(buffer, sizeof(char), records_length, file) != records_length) {

                perror("error occurred while writing records into file\n");
                exit(1);
        }
    }

    // Closing file
    if(fclose(file) != 0) {

        perror("error occurred while closing file\n");
        exit(1);
    }
}

// Copies file
void copy(int argc, char** argv) {

    if(argc < 7) {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }

    // Parsing all necessary arguments
    char* input_filename = argv[2];
    char* output_filename = argv[3];
    int records_number = atoi(argv[4]);
    int buffer_size = atoi(argv[5]);
    char* mode = argv[6];

    char* buffer = malloc(buffer_size * sizeof(char));

    // Copying file using C standard library functions
    if(strcmp(mode, "lib") == 0) {

        // Opening both input and output files
        FILE* input_file = fopen(input_filename, "r");
        FILE* output_file = fopen(output_filename, "w");

        if(input_file == NULL || output_file == NULL) {

            perror("error occurred while opening file\n");
            exit(1);
        }

        // Reading from input and writing to output record by record
        for(int i = 0; i < records_number; i++) {

            if(fread(buffer, sizeof(char), buffer_size, input_file) != buffer_size){

                    perror("error occurred while reading from file\n");
                    exit(1);
            }

            if(fwrite(buffer, sizeof(char), buffer_size, output_file) != buffer_size){

                    perror("error occurred while writing into file\n");
                    exit(1);
            }
        }

        // Closing files
        if(fclose(input_file) != 0 || fclose(output_file) != 0) {

            perror("error occurred while closing file\n");
            exit(1);
        }

    }

    // Copying file using C system call functions
    else if(strcmp(mode, "sys") == 0) {

        // Opening input file
        int input_fd = open(input_filename, O_RDONLY);

        // Opening output file
        // If the output file does not exist it is created with the following permissions: owner:rw, group:r, other:r
        int output_fd = open(output_filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if(input_fd < 0 || output_fd < 0) {

            perror("error occurred while opening file\n");
            exit(1);
        }

        // Reading from input and writing to output record by record
        for(int i = 0; i < records_number; i++) {

            if(read(input_fd, buffer, buffer_size * sizeof(char)) != buffer_size) {

                    perror("error occurred while reading from file\n");
                    exit(1);
            }

            if(write(output_fd, buffer, buffer_size * sizeof(char)) != buffer_size) {

                    perror("error occurred while writing into file\n");
                    exit(1);
            }
        }

        // Closing files
        if(close(input_fd) < 0 || close(output_fd) < 0) {

            perror("error occurred while closing file\n");
            exit(1);
        }

    }

    // In case none of valid modes was recognized
    else {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }
}

// Sorts records in a file with insertion sort algorithm
void sort(int argc, char** argv) {

    if(argc < 6) {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }

    // Parsing all necessary arguments
    char* filename = argv[2];
    int records_number = atoi(argv[3]);
    int records_length = atoi(argv[4]);
    char* mode = argv[5];

    // Creating two buffers for storing records in RAM (no more than two at the same time)
    char* uninserted = malloc(records_length * sizeof(char));
    char* swap_buffer = malloc(records_length * sizeof(char));

    // Sorting file using C standard library functions
    if(strcmp(mode, "lib") == 0) {

        // Opening file
        FILE* file = fopen(filename, "r+");

        if(file == NULL) {

            perror("error occurred while opening file\n");
            exit(1);
        }

        // Iterating through all records in file
        for(int i = 1; i < records_number; i++) {

            // Setting file pointer to the beginning of the next record
            int next_record_position = records_length * i;
            int target_position = 0;

            if(fseek(file, next_record_position * sizeof(char), SEEK_SET) != 0) {

                perror("error occurred while setting file pointer\n");
                exit(1);
            }

            // Loading next record from file into 'uninserted' array
            if(fread(uninserted, sizeof(char), records_length, file) != records_length) {

                perror("error occurred while reading from file\n");
                exit(1);
            }

            // Searching for a proper position for the record
            for(int j = i; j > 0; j--) {

                int record_iterator = records_length * j;

                // Loading record from file into 'swap_buffer' array to change its position in file
                if(fseek(file, (record_iterator - records_length) * sizeof(char), SEEK_SET) != 0) {

                    perror("error occurred while setting file pointer\n");
                    exit(1);
                }
                if(fread(swap_buffer, sizeof(char), records_length, file) != records_length) {

                    perror("error occurred while reading from file\n");
                    exit(1);
                }

                // If proper position is found -> ends the inner loop
                if((unsigned char)uninserted[0] >= (unsigned char)swap_buffer[0]) {

                    target_position = record_iterator;
                    break;
                }

                // If not -> rewrites previously read record to other place in file
                else {

                    if(fseek(file, record_iterator * sizeof(char), SEEK_SET) != 0) {

                        perror("error occurred while setting file pointer\n");
                        exit(1);
                    }

                    if(fwrite(swap_buffer, sizeof(char), records_length, file) != records_length){

                        perror("error occurred while writing into file\n");
                        exit(1);
                    }
                }
            }

            // Inserting record into its proper position in file after finishing inner loop
            if(fseek(file, target_position * sizeof(char), SEEK_SET) != 0) {

                perror("error occurred while setting file pointer\n");
                exit(1);
            }
            if(fwrite(uninserted, sizeof(char), records_length, file) != records_length){

                perror("error occurred while writing into file\n");
                exit(1);
            }
        }

        // Closing file
        if(fclose(file) != 0) {

            perror("error occurred while closing file\n");
            exit(1);
        }
    }

    // Sorting with C system call functions
    else if(strcmp(mode, "sys") == 0) {

        // Opening file
        int file_desc = open(filename, O_RDWR);

        if(file_desc < 0) {

            perror("error occurred while opening file\n");
            exit(1);
        }

        // Iterating through all records in file
        for(int i = 1; i < records_number; i++) {

            // Setting file pointer to the beginning of the next record
            int next_record_position = records_length * i;
            int target_position = 0;
            if(lseek(file_desc, next_record_position * sizeof(char), SEEK_SET) < 0) {

                perror("error occurred while setting file pointer\n");
                exit(1);
            }

            // Loading next record from file into 'uninserted' array
            if(read(file_desc, uninserted, records_length * sizeof(char)) != records_length * sizeof(char)) {

                perror("error occurred while reading from file\n");
                exit(1);
            }

            // Searching for a proper position for the record
            for(int j = i; j > 0; j--) {

                int record_iterator = records_length * j;

                // Loading record from file into 'swap_buffer' array to change its position in file
                if(lseek(file_desc, (record_iterator - records_length) * sizeof(char), SEEK_SET) < 0) {

                    perror("error occurred while setting file pointer\n");
                    exit(1);
                }
                if(read(file_desc, swap_buffer, records_length * sizeof(char)) != records_length * sizeof(char)) {

                    perror("error occurred while reading from file\n");
                    exit(1);
                }

                // If proper position is found -> ends the inner loop
                if((unsigned char)uninserted[0] >= (unsigned char)swap_buffer[0]) {

                    target_position = record_iterator;
                    break;
                }

                // If not -> rewrites previously read record to other place in file
                else {

                    if(lseek(file_desc, record_iterator * sizeof(char), SEEK_SET) < 0) {

                        perror("error occurred while setting file pointer\n");
                        exit(1);
                    }
                    if(write(file_desc, swap_buffer, records_length * sizeof(char)) != records_length * sizeof(char)){

                        perror("error occurred while writing into file\n");
                        exit(1);
                    }
                }
            }

            // Inserting record into its proper position in file after finishing inner loop
            if(lseek(file_desc, target_position * sizeof(char), SEEK_SET) < 0) {

                perror("error occurred while setting file pointer\n");
                exit(1);
            }
            if(write(file_desc, uninserted, records_length * sizeof(char)) != records_length * sizeof(char)){

                perror("error occurred while writing into file\n");
                exit(1);
            }
        }

        // Closing file
        if(close(file_desc) != 0) {

            perror("error occurred while closing file\n");
            exit(1);
        }
    }

    // None of both modes was recognized

    else {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }

}

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

int main(int argc, char** argv) {

    srand(time(NULL));

    struct rusage ru_start, ru_end;
    struct timeval sys_start, sys_end, user_start, user_end;
    clock_t real_start, real_end;

    // Measuring start time
    real_start = clock();
    getrusage(RUSAGE_SELF, &ru_start);

    if(argc == 1) {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }

    if(strcmp(argv[1], "generate") == 0) {

        generate(argc, argv);
    }

    else if(strcmp(argv[1], "copy") == 0) {

        copy(argc, argv);
    }

    else if(strcmp(argv[1], "sort") == 0) {

        sort(argc, argv);
    }

    else {

        fprintf(stderr, "unknown command: %s\n", argv[1]);
        exit(1);
    }

    // Measuring end time
    real_end = clock();
    getrusage(RUSAGE_SELF, &ru_end);

    sys_start = ru_start.ru_stime;
    user_start = ru_start.ru_utime;
    sys_end = ru_end.ru_stime;
    user_end = ru_end.ru_utime;

    printf("execution time:\n");
    printf("real\t");
    print_time(((double) real_end - real_start) / CLOCKS_PER_SEC);
    printf("user\t");
    print_time(subtract_time(user_end, user_start));
    printf("sys\t");
    print_time(subtract_time(sys_end, sys_start));
}
