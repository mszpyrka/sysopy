#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

int main_loop_active;

// Prints current time to stdio
void print_time() {

    time_t int_time;
    time(&int_time);
    struct tm *current_time = localtime(&int_time);

    printf("%s\n", asctime(current_time));
    fflush(stdout); // To ensure that the date is printed at this exact point of program execution
}


// Function used for handling SIGINT
void sig_int(int signo) {

    printf("Odebrano sygnal SIGINT\n");
    exit(0);
}

int main(int argc, char** argv) {

    if(signal(SIGINT, sig_int) == SIG_ERR) {

        perror("Cannot set SIGINT signal handler");
        exit(1);
    }

    while(1) {

        print_time();
        sleep(1);
    }
}
