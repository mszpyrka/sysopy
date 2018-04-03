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


// Forks and executes passed script in child process
void execute_script() {

    pid_t pid = fork();

    if(pid < 0)
        perror("Error occurred while executing fork()");

    else if(pid > 0)
        child_proc_id = pid;

    else {
        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

        if(execlp(name, name, NULL) == -1) {
            perror("Error occured while trying to execute script");
            exit(1);
        }
    }
}


// Ends child process, returns 0 on success, -1 if there were no children processes
int end_child_process() {

    if(child_proc_id == -1)
        return -1;

    pid_t info = waitpid(child_proc_id, NULL, WNOHANG);


    if(info == 0) {    // When child process is executing

        kill(child_proc_id, SIGKILL);

        child_proc_id = -1;
        return 0;
    }

    // When there are no children processes
    child_proc_id = -1;
    return -1;
}


// Function used for handling SIGINT
void sig_int(int signo) {

    end_child_process();
    printf("Odebrano sygnal SIGINT\n");
    exit(0);
}


// Function used for handling SIGTSTP
void sig_tstp(int signo) {

    if(end_child_process() == 0) {

        printf("Oczekuje na CTRL+Z - kontynuacja albo CTRL+C - zakonczenie programu\n");
        fflush(stdout);
    }

    else
        execute_script();
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

    // struct sigaction setup
    struct sigaction act_chld;
    act_chld.sa_handler = SIG_DFL;
    sigemptyset(&act_chld.sa_mask);
    act_chld.sa_flags = SA_NOCLDWAIT;

    // Setting handler for SIGTSTP
    if(sigaction(SIGCHLD, &act_chld, NULL) == -1) {

        perror("Cannot set SIGCHLD signal handler");
        exit(1);
    }

    execute_script();

    while(1)
        pause();
}
