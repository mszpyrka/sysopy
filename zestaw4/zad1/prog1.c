#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>


// Flag that stores the current program mode (whether it should be running or waiting for SIGTSTP to continue)
int main_loop_active;

// Buffer storing time string
char buffer[50];


// Prints current time to stdio
void print_time() {

    time_t int_time;
    time(&int_time);
    struct tm *current_time = localtime(&int_time);

    strftime(buffer, 50, "%H:%M:%S", current_time);
    printf("time: %s\n", buffer);
    fflush(stdout); // To ensure that the date is printed at this exact point of program execution
}


// Function used for handling SIGINT
void sig_int(int signo) {

    printf("Odebrano sygnal SIGINT\n");
    exit(0);
}


// Function used for handling SIGTSTP
void sig_tstp(int signo) {

    if(main_loop_active == 1) {

        main_loop_active = 0;
        printf("Oczekuje na CTRL+Z - kontynuacja albo CTRL+C - zakonczenie programu\n");
        fflush(stdout);
    }

    else
        main_loop_active = 1;
}


int main(int argc, char** argv) {

    // Setting handler for SIGINT
    if(signal(SIGINT, sig_int) == SIG_ERR) {

        perror("Cannot set SIGINT signal handler");
        exit(1);
    }

    // struct sigaction setup
    struct sigaction act_tstp;
    act_tstp.sa_handler = sig_tstp;
    sigemptyset(&act_tstp.sa_mask);
    act_tstp.sa_flags = 0;

    // Setting handler for SIGTSTP
    if(sigaction(SIGTSTP, &act_tstp, NULL) == -1) {

        perror("Cannot set SIGTSTP signal handler");
        exit(1);
    }

    main_loop_active = 1;

    // Infinite loop
    while(1) {

        if(main_loop_active == 1) {

            print_time();
            sleep(1);
        }

        else
            pause();
    }
}
