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
    fprintf(stdout, "time: %s\n", buffer);
}


// Function used for handling SIGINT
void sig_int(int signo) {

    fprintf(stdout, "Odebrano sygnal SIGINT\n");
    exit(0);
}


// Function used for handling SIGTSTP
void sig_tstp(int signo) {

    // In case handler was called from previous call to this handler
    if(main_loop_active == 0) {
        main_loop_active = 1;
        return;
    }

    // In case handler was called while processing main loop
    main_loop_active = 0;
    fprintf(stdout, "Oczekuje na CTRL+Z - kontynuacja albo CTRL+C - zakonczenie programu\n");

    sigset_t wait_mask;
    sigfillset(&wait_mask);
    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGTSTP);
    sigsuspend(&wait_mask); // Pauses until SIGTSTP or SIGINT occurs
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

        print_time();
        sleep(1);
    }
}
