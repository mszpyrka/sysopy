#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char* argv[]) {

    if(argc < 2)
        exit(1);

    if(strcmp(argv[1], "mem") == 0) {

        if(argc < 3)
            exit(1);

        int chunk_size = ((long int) atoi(argv[2])) << 20; // argv[2] specifies memory in Mb to allocate

        char* buffer = calloc(chunk_size, sizeof(char));
        buffer[0] = 'a';    // To prevent warnings about unused variable
    }

    else if(strcmp(argv[1], "cpu") == 0) {
        long int sum = 0;

        while(1) // Infinite calculations
            sum++;
    }

    printf("resources wasted successfully!\n");
}
