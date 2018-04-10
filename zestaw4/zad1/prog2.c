#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


// Stores the id of child process
pid_t child_proc_id;
const char *name;

int main_loop_active;


// Forks and executes passed script in child process
void execute_script() {

    pid_t pid = fork();

    if(pid < 0)
        perror("Error occurred while executing fork()");

    else if(pid > 0)
        child_proc_id = pid;

    else {
        // Ignores both signals that would be sent to process from shell if user typed keyboard signal keys
        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

        if(execlp(name, name, NULL) == -1) {
            perror("Error occured while trying to execute script");
            exit(1);
        }
    }
}


// Function used for handling SIGINT
void sig_int(int signo) {

    if(main_loop_active == 1)
        kill(child_proc_id, SIGKILL);

    printf("Odebrano sygnal SIGINT\n");
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
    kill(child_proc_id, SIGKILL);
    main_loop_active = 0;
    fprintf(stdout, "Oczekuje na CTRL+Z - kontynuacja albo CTRL+C - zakonczenie programu\n");

    sigset_t wait_mask;
    sigfillset(&wait_mask);
    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGTSTP);
    sigsuspend(&wait_mask); // Pauses until SIGTSTP or SIGINT occurs
}


// Prints example program usage
void print_usage() {
    printf("example usage: ./prog <shell_script_path>\n");
}


int main(int argc, char** argv) {

    if(argc < 2) {
        print_usage();
        exit(1);
    }

    child_proc_id = -1;
    name = argv[1];

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

    while(1) {
        execute_script();
        pause();
    }
}
