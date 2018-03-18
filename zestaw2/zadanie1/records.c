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

// Fills passed array with randomly generated values (in this case -> char values)
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

                fprintf(stderr, "error occurred while writing records into file\n");
                exit(1);
        }

    }

    // Closing file
    if(fclose(file) != 0) {

        fprintf(stderr, "error occurred while closing file\n");
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

            fprintf(stderr, "error occurred while opening file\n");
            exit(1);
        }

        // Reading from input and writing to output record by record
        for(int i = 0; i < records_number; i++) {

            if(fread(buffer, sizeof(char), buffer_size, input_file) != buffer_size){

                    fprintf(stderr, "error occurred while reading from file\n");
                    exit(1);
            }

            if(fwrite(buffer, sizeof(char), buffer_size, output_file) != buffer_size){

                    fprintf(stderr, "error occurred while writing into file\n");
                    exit(1);
            }
        }

        // Closing files
        if(fclose(input_file) != 0 || fclose(output_file) != 0) {

            fprintf(stderr, "error occurred while closing file\n");
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

            fprintf(stderr, "error occurred while opening file\n");
            exit(1);
        }

        // Reading from input and writing to output record by record
        for(int i = 0; i < records_number; i++) {

            if(read(input_fd, buffer, buffer_size * sizeof(char)) != buffer_size) {

                    fprintf(stderr, "error occurred while reading from file\n");
                    exit(1);
            }

            if(write(output_fd, buffer, buffer_size * sizeof(char)) != buffer_size) {

                    fprintf(stderr, "error occurred while writing into file\n");
                    exit(1);
            }
        }

        // Closing files
        if(close(input_fd) < 0 || close(output_fd) < 0) {

            fprintf(stderr, "error occurred while closing file\n");
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

            fprintf(stderr, "error occurred while opening file\n");
            exit(1);
        }

        // Iterating through all records in file
        for(int i = 1; i < records_number; i++) {

            // Setting file pointer to the beginning of the next record
            int next_record_position = records_length * i;
            int target_position = 0;
            fseek(file, next_record_position * sizeof(char), SEEK_SET);

            // Loading next record from file into 'uninserted' array
            fread(uninserted, sizeof(char), records_length, file);

            // Searching for a proper position for the record
            for(int j = i; j > 0; j--) {

                int record_iterator = records_length * j;

                // Loading record from file into 'swap_buffer' array to change its position in file
                fseek(file, (record_iterator - records_length) * sizeof(char), SEEK_SET);
                fread(swap_buffer, sizeof(char), records_length, file);

                // If proper position is found -> ends the inner loop
                if(uninserted[0] > swap_buffer[0]) {

                    target_position = record_iterator;
                    break;
                }

                // If not -> rewrites previously read record to other place in file
                else {

                    fseek(file, record_iterator * sizeof(char), SEEK_SET);

                    if(fwrite(swap_buffer, sizeof(char), records_length, file) != records_length){

                        fprintf(stderr, "error occurred while writing into file\n");
                        exit(1);
                    }
                }
            }

            // Inserting record into its proper position in file after finishing inner loop
            fseek(file, target_position * sizeof(char), SEEK_SET);
            fwrite(uninserted, sizeof(char), records_length, file);
        }

        // Closing file
        fclose(file);
    }

    // Sorting with C system call functions
    else if(strcmp(mode, "sys") == 0) {

        // Opening file
        int file_desc = open(filename, O_RDWR);

        if(file_desc < 0) {

            fprintf(stderr, "error occurred while opening file\n");
            exit(1);
        }

        // Iterating through all records in file
        for(int i = 1; i < records_number; i++) {

            // Setting file pointer to the beginning of the next record
            int next_record_position = records_length * i;
            int target_position = 0;
            lseek(file_desc, next_record_position * sizeof(char), SEEK_SET);

            // Loading next record from file into 'uninserted' array
            read(file_desc, uninserted, records_length * sizeof(char));

            // Searching for a proper position for the record
            for(int j = i; j > 0; j--) {

                int record_iterator = records_length * j;

                // Loading record from file into 'swap_buffer' array to change its position in file
                lseek(file_desc, (record_iterator - records_length) * sizeof(char), SEEK_SET);
                read(file_desc, swap_buffer, records_length * sizeof(char));

                // If proper position is found -> ends the inner loop
                if(uninserted[0] > swap_buffer[0]) {

                    target_position = record_iterator;
                    break;
                }

                // If not -> rewrites previously read record to other place in file
                else {

                    lseek(file_desc, record_iterator * sizeof(char), SEEK_SET);

                    if(write(file_desc, swap_buffer, records_length * sizeof(char)) != records_length){

                        fprintf(stderr, "error occurred while writing into file\n");
                        exit(1);
                    }
                }
            }

            // Inserting record into its proper position in file after finishing inner loop
            lseek(file_desc, target_position * sizeof(char), SEEK_SET);
            write(file_desc, uninserted, records_length * sizeof(char));
        }

        // Closing file
        close(file_desc);
    }

    // None of both modes was recognized

    else {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }

}

int main(int argc, char** argv) {

    srand(time(NULL));

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
}
