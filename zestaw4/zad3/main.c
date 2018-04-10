#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int TRIGGER_SIGNAL;
int RESPONSE_SIGNAL;
int TERMINATION_SIGNAL;

pid_t child_pid;
int sent_trigger_signals;       // Counts number of signals sent from parent process
int received_response_signals;  // Counts signals received from child by parent process
int ch_received_signals;    // Contains number of signals received by child process
int responded;              // Flag to check if parent process received the child's response for the last TRIGGER_SIGNAL

// ============================================================================================================================================
// This section contains different signal handlers (both for parent and child processes)
// ============================================================================================================================================


// Function used for handling trigger signals in child process
void ch_sig_trigger(int signo) {

    ch_received_signals++;
    kill(getppid(), RESPONSE_SIGNAL);
}


// Function used to handle termination signal in child process
void ch_sig_term(int signo) {

    fprintf(stdout, "child process: received %d trigger signals from parent\n", ch_received_signals);
    exit(0);
}


// Function used for handling SIGINT
void sig_int(int signo) {

    printf("Received signal SIGINT\n");

    kill(child_pid, TERMINATION_SIGNAL);

    fprintf(stdout, "parent process: sent %d trigger signals to child\n", sent_trigger_signals);
    fprintf(stdout, "parent process: received %d response signals from child\n", received_response_signals);
    exit(1);
}


// Function used for handling RESPONSE_SIGNAL
void sig_response(int signo) {

    responded = 1;
    received_response_signals++;
}


// ============================================================================================================================================
// ============================================================================================================================================


// Sets signal handlers for parent process
void set_parent_handlers() {

    // struct sigaction setup
    struct sigaction s_act;
    s_act.sa_handler = sig_int;
    sigfillset(&s_act.sa_mask);

    // Setting handler for SIGINT
    if(sigaction(SIGINT, &s_act, NULL) == -1) {

        perror("Cannot set SIGINT signal handler");
        exit(1);
    }

    s_act.sa_handler = sig_response;
    sigfillset(&s_act.sa_mask);
    sigdelset(&s_act.sa_mask, SIGINT);

    // Setting handler for RESPONSE_SIGNAL
    if(sigaction(RESPONSE_SIGNAL, &s_act, NULL) == -1) {

        perror("Cannot set response signal handler");
        exit(1);
    }
}


// Sets signal handlers for child process
void set_child_handlers() {

    // struct sigaction setup
    struct sigaction s_act;
    s_act.sa_handler = ch_sig_term;
    sigfillset(&s_act.sa_mask);

    // Setting handler for TERMINATION_SIGNAL
    if(sigaction(TERMINATION_SIGNAL, &s_act, NULL) == -1) {

        perror("Cannot set termination signal handler");
        exit(1);
    }

    sigdelset(&s_act.sa_mask, TERMINATION_SIGNAL);
    s_act.sa_handler = ch_sig_trigger;

    // Setting handler for TRIGGER_SIGNAL
    if(sigaction(TRIGGER_SIGNAL, &s_act, NULL) == -1) {

        perror("Cannot set trigger signal handler");
        exit(1);
    }
}


// Function executed by every child process
void child_proc_fun() {

    ch_received_signals = 0;
    set_child_handlers();

    // Unblocks TRIGGER_SIGNAL and TERMINATION_SIGNAL
    sigset_t child_mask;
    sigfillset(&child_mask);
    sigdelset(&child_mask, TRIGGER_SIGNAL);
    sigdelset(&child_mask, TERMINATION_SIGNAL);

    while(1)
        sigsuspend(&child_mask);
}


// Creates new child process, saves child's id
void create_child() {

    sigset_t child_mask, parent_mask;
    sigfillset(&child_mask);

    // Sets sigmask for child process (initially blocks all signals)
    if(sigprocmask(SIG_SETMASK, &child_mask, &parent_mask) == -1) {

        perror("Could not set sigmask");
        exit(1);
    }

    pid_t id = fork();

    if(id == 0)
        child_proc_fun();

    child_pid = id;

    if(sigprocmask(SIG_SETMASK, &parent_mask, NULL) == -1) {

        perror("Could not restore previous sigmask");
        exit(1);
    }
}


// Prints example program usage
void print_usage() {
    printf("example usage: ./prog <signals_number> <type>\n");
}


int main(int argc, char** argv) {

    if(argc < 3) {
        print_usage();
        exit(1);
    }

    int signals_to_send = atoi(argv[1]);
    int program_type = atoi(argv[2]);

    sent_trigger_signals = 0;
    received_response_signals = 0;

    if(program_type == 1) {

        TRIGGER_SIGNAL = SIGUSR1;
        RESPONSE_SIGNAL = SIGUSR1;
        TERMINATION_SIGNAL = SIGUSR2;

        create_child();
        set_parent_handlers();

        for(int i = 0; i < signals_to_send; i++) {
            kill(child_pid, TRIGGER_SIGNAL);
            sent_trigger_signals++;
        }

        kill(child_pid, TERMINATION_SIGNAL);
    }

    else if(program_type == 2) {

        TRIGGER_SIGNAL = SIGUSR1;
        RESPONSE_SIGNAL = SIGUSR1;
        TERMINATION_SIGNAL = SIGUSR2;

        create_child();
        set_parent_handlers();

        sigset_t response_mask, prev_mask;
        sigemptyset(&response_mask);
        sigaddset(&response_mask, RESPONSE_SIGNAL);

        if(sigprocmask(SIG_BLOCK, &response_mask, &prev_mask) == -1) {

            perror("Could not set sigmas");
            exit(1);
        }

        for(int i = 0; i < signals_to_send; i++) {

            responded = 0;

            kill(child_pid, TRIGGER_SIGNAL);
            sent_trigger_signals++;

            while(responded == 0)
                sigsuspend(&prev_mask);
        }

        kill(child_pid, TERMINATION_SIGNAL);
    }

    else if(program_type == 3) {

        TRIGGER_SIGNAL = SIGRTMIN;
        RESPONSE_SIGNAL = SIGRTMIN;
        TERMINATION_SIGNAL = SIGRTMAX;

        create_child();
        set_parent_handlers();

        for(int i = 0; i < signals_to_send; i++) {
            kill(child_pid, TRIGGER_SIGNAL);
            sent_trigger_signals++;
        }

        kill(child_pid, TERMINATION_SIGNAL);
    }

    else {
        fprintf(stderr, "wrong argument format\n");
        print_usage();
    }

    fprintf(stdout, "parent process: sent %d trigger signals to child\n", sent_trigger_signals);
    fprintf(stdout, "parent process: received %d response signals from child\n", received_response_signals);
}
