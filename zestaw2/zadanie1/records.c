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

void generate_random_record(char* buffer, int bytes_number) {

    for(int i = 0; i < bytes_number; i++)
        buffer[i] = (char)(rand() % 95 + 33); // all ascii codes of non-white characters
}

// Creates file with randomly generated records
void generate(int argc, char** argv) {

    if(argc < 5) {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }

    char* filename = argv[2];
    int records_number = atoi(argv[3]);
    int records_length = atoi(argv[4]);

    FILE* file = fopen(filename, "w");

    if(file == NULL) {

        fprintf(stderr, "error occurred while opening file\n");
        exit(1);
    }

    char* buffer = malloc(records_length * sizeof(char));

    for(int i = 0; i < records_number; i++) {

        generate_random_record(buffer, records_length);

        if(fwrite(buffer, sizeof(char), records_length, file) != records_length) {

                fprintf(stderr, "error occurred while writing records into file\n");
                exit(1);
        }

    }

    if(fclose(file) != 0) {

        fprintf(stderr, "error occurred while closing file\n");
        exit(1);
    }
}

// Copies file
void copy(int argc, char** argv) {

    if(argc < 6) {

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

        FILE* input_file = fopen(input_filename, "r");
        FILE* output_file = fopen(output_filename, "w");

        if(input_file == NULL || output_file == NULL) {

            fprintf(stderr, "error occurred while opening file\n");
            exit(1);
        }

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

        if(fclose(input_file) != 0 || fclose(output_file) != 0) {

            fprintf(stderr, "error occurred while closing file\n");
            exit(1);
        }

    }

    // Copying file using C system call functions

    else if(strcmp(mode, "sys") == 0) {

        int input_fd = open(input_filename, O_RDONLY);

        // If the output file does not exist it is created with following permissions: owner:rw, group:r, other:r
        int output_fd = open(output_filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if(input_fd < 0 || output_fd < 0) {

            fprintf(stderr, "error occurred while opening file\n");
            exit(1);
        }

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

        if(close(input_fd) < 0 || close(output_fd) < 0) {

            fprintf(stderr, "error occurred while closing file\n");
            exit(1);
        }

    }

    // None of both modes was recognized

    else {

        fprintf(stderr, "wrong arguments format\n");
        exit(1);
    }
}

void sort(int argc, char** argv) {



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
