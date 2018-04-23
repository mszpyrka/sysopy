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
    printf("example usage: ./slave <path_to_fifo> N\n");
}

int main(int argc, char **argv) {

    srand(getpid());

    if(argc != 3) {
        print_usage();
        exit(1);
    }

    fprintf(stdout, "starting slave process: %d\n", getpid());

    int fifo_fd;

    if((fifo_fd = open(argv[1], O_WRONLY)) < 0) {

        perror("Error occurred while opening fifo");
        exit(1);
    }

    int N = atoi(argv[2]);

    size_t buf_size = 0;

    char *buffer1 = NULL;
    char *buffer2 = malloc(1000 * sizeof(char));

    FILE *date_fp;

    for(int i = 0; i < N; i++) {

        if((date_fp = popen("date", "r")) == NULL) {

            perror("Error occurred while using popen");
            exit(1);
        }

        if(getline(&buffer1, &buf_size, date_fp) < 0) {

            perror("Error occurred while reading date");
            exit(1);
        }

        sprintf(buffer2, "%d: %s", getpid(), buffer1);

        if(write(fifo_fd, buffer2, strlen(buffer2) * sizeof(char)) < 0) {

            perror("Error occurred while writing to fifo");
            exit(1);
        }

        if(pclose(date_fp) == -1) {

            perror("Error occurred while using pclose");
            exit(1);
        }

        int rand_sec = rand() % 4 + 2;

        sleep(rand_sec);
    }

    close(fifo_fd);
}
