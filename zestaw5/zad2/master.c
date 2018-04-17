#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


void print_usage() {
    printf("example usage: ./master <path_to_fifo>\n");
}

int main(int argc, char **argv) {

    if(argc != 2) {
        print_usage();
        exit(1);
    }

    if(mkfifo(argv[1], S_IRUSR | S_IWUSR) == -1) {

        perror("Error occurred while creating fifo");
        exit(1);
    }

    FILE *fifo_fp;

    if((fifo_fp = fopen(argv[1], "r")) == NULL) {

        perror("Error occurred while opening fifo");
        exit(1);
    }


    size_t buf_size = 0;
    char *buffer = NULL;

    while(getline(&buffer, &buf_size, fifo_fp) > 0)
        fprintf(stdout, "%s", buffer);

    fclose(fifo_fp);

    if(remove(argv[1]) == -1) {

        perror("Error occurred while deleting fifo");
        exit(1);
    }
}
