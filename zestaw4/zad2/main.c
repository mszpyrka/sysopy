#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

// Linux doesn't support queuing non-real-time signals
// Defining SIGUSR1 as REQUEST_SIGNAL (as recommended in task instruction) will result in signal losses in some cases
// Better alternative is to use SIGRTMAX (all other real-time signals are used in this program, so using them will lead to complete failure)
#define REQUEST_SIGNAL SIGRTMAX

#define RED         "\x1B[31m"
#define GREEN       "\x1B[32m"
#define YELLOW      "\x1B[33m"
#define BLUE        "\x1B[34m"
#define MAGENTA     "\x1B[35m"
#define CYAN        "\x1B[36m"
#define WHITE       "\x1B[37m"
#define RESET       "\x1B[0m"

// Options defining which values the main program should track during the execution
int track_created = 1;
int track_requests = 1;
int track_approvals = 1;
int track_rtsignals = 1;
int track_exit_codes = 1;

// Structure storing information about signals received from children precesses
struct received_signal {

    int signal;
    pid_t pid;
};

// Struct storing information about children exit codes
struct exit_code {

    int code;
    pid_t pid;
};

// Arrays for storing data about tracked features
int created_it;
pid_t *created_processes;

int requests_it;
pid_t *received_requests;

int approvals_it;
pid_t *sent_approvals;

int received_it;
struct received_signal *received_rtsignals;

int exit_it;
struct exit_code *exit_codes;

// Program parameters (number of children and number of requests to receive before sending approvals)
int children_number;
int requests_threshold;

// Denotes whether all created child processes have finished their execution
int all_children_finished;



// ============================================================================================================================================
// This section of code is used only by children processes
// ============================================================================================================================================

// Flag that contains information whether the parent process has already approved child's request or not
int request_approved;

void ch_sig_usr2(int signo) {

    if(track_requests == 1)
        fprintf(stderr, YELLOW "child %d: received parent's approval\n", getpid());

    request_approved = 1;
}

void ch_sig_quit(int signo) {

    exit(22);
}

// Function executed by every child process
void child_proc_fun(int seed) {

    sigset_t child_proc_mask;
    sigfillset(&child_proc_mask);
    sigdelset(&child_proc_mask, SIGQUIT);
    sigdelset(&child_proc_mask, SIGUSR2);

    // Sets initial sigmask
    if(sigprocmask(SIG_SETMASK, &child_proc_mask, NULL) == -1) {

        fprintf(stderr, "Could not set sigmask in child process %d", getpid());
        perror("");
        exit(20);
    }

    // Handlers block all signals except SIGQUIT
    struct sigaction s_act;
    sigfillset(&s_act.sa_mask);
    sigdelset(&s_act.sa_mask, SIGQUIT);

    // Setting handler for SIGINT
    s_act.sa_handler = SIG_IGN;   // Ignores interrupt signal in order not to end the process in case of interactive terminal interrupt
    if(sigaction(SIGINT, &s_act, NULL) == -1) {

        perror("Cannot set SIGINT signal handler for child process");
        exit(20);
    }

    // Setting handler for SIGUSR2
    s_act.sa_handler = ch_sig_usr2;
    if(sigaction(SIGUSR2, &s_act, NULL) == -1) {

        perror("Cannot set SIGUSR2 signal handler for child process");
        exit(20);
    }

    // Setting handler for SIGQUIT
    s_act.sa_handler = ch_sig_quit;
    if(sigaction(SIGQUIT, &s_act, NULL) == -1) {

        perror("Cannot set SIGQUIT signal handler for child process");
        exit(20);
    }


    srand(seed);
    int sleep_time = rand() % 11; // Randomly chooses number between 0 and 10

    sleep(sleep_time);

    union sigval none;

    sigset_t request_mask, old_mask;
    sigfillset(&request_mask);
    sigdelset(&request_mask, SIGQUIT);

    // Blocks REQUEST_SIGNAL for critical part of code
    if(sigprocmask(SIG_BLOCK, &request_mask, &old_mask) == -1) {

        fprintf(stderr, "Could not set sigmask in child process %d", getpid());
        perror("");
        exit(20);
    }

    // Sends request to parent process
    if(sigqueue(getppid(), REQUEST_SIGNAL, none) == -1) {

        fprintf(stderr, "Could not send REQUEST_SIGNAL to parent process %d", getppid());
        perror("");
        exit(20);
    }

    if(track_requests == 1)
        fprintf(stdout, YELLOW "child %d: successfully sent REQUEST_SIGNAL to parent process %d\n" RESET, getpid(), getppid());

    request_approved = 0;

    // Unblocks REQUEST_SIGNAL
    while(request_approved == 0)
        sigsuspend(&old_mask);

    // Sends random real-time signal (excluding SIGRTMAX) to parent process
    int rand_sig = SIGRTMIN + rand() % (SIGRTMAX - SIGRTMIN);

    if(sigqueue(getppid(), rand_sig, none) == -1) {

        fprintf(stderr, "Could not send SIGRT to parent process %d", getppid());
        perror("");
        exit(20);
    }

    if(track_rtsignals == 1)
        fprintf(stdout, YELLOW "child %d: successfully sent RTSIG %d to parent process %d\n" RESET, getpid(), rand_sig, getppid());

    exit(sleep_time);
}

// ============================================================================================================================================
// ============================================================================================================================================


// Creates new child process, saves child's id in created_processes array
void create_child() {

    pid_t id = fork();

    if(id == 0)
        child_proc_fun(getpid()); // new pid is sent as rand seed to child_proc_fun in order to guarantee different seeds across all children

    created_processes[created_it] = id;
    created_it++;

    if(track_created == 1)
        fprintf(stdout, GREEN "parent: created child process %d\n" RESET, id);
}


// Sends SIGQUIT to child precess if one has not exited yet
void kill_child(pid_t child_id) {

    // Checks if given child process has already finished
    for(int i = 0; i < exit_it; i++)
        if(exit_codes[i].pid == child_id)
            return;

    kill(child_id, SIGQUIT);
}


// Sends SIGUSR2 to child precess to allow it to continue its work
void send_single_approval(pid_t child_pid) {

    kill(child_pid, SIGUSR2);

    if(track_approvals == 1) {

        sent_approvals[approvals_it] = child_pid;
        approvals_it++;

        fprintf(stdout, CYAN "parent: sent request approval to child process %d\n" RESET, child_pid);
    }
}


// Function used for handling SIGINT
void sig_int(int signo) {

    fprintf(stdout, RED "Received signal SIGINT\n" RESET);

    for(int i = 0; i < created_it; i++)
        kill_child(created_processes[i]);

    all_children_finished = -1;
}


// Function used for handling SIGCHLD
// This handler assumes that it is being called only after child process termination, thus should be installed with SA_NOCHLDSTOP flag
void sig_chld(int signo) {

    int status;
    int child_id;

    // Saves info about processes that sent the signal
    while((child_id = waitpid(-1, &status, WNOHANG)) > 0) {
        exit_codes[exit_it].pid = child_id;
        exit_codes[exit_it].code = WEXITSTATUS(status);
        exit_it++;

        if(track_exit_codes == 1)
            fprintf(stdout, RED "parent: child process %d terminated with exit code %d\n" RESET, child_id, WEXITSTATUS(status));
    }

    // Exits if all children have finished their processes and all signals have beed processed successfully
    if(exit_it == children_number)
        all_children_finished = 1;
}


// Function used for handling REQUEST_SIGNAL (makes use of siginfo structure, should be installed via sigaction with SA_SIGINFO flag enabled)
// This handler sends approvals to all processes that sent their request when the threshold is reached, so should block all signals that modify received request data
void sig_req(int signo, siginfo_t *info, void *ucontext) {

    if(track_requests == 1) {
        received_requests[requests_it] = info -> si_pid;
        fprintf(stdout, BLUE "parent: received REQUEST_SIGNAL from child process %d\n" RESET, info -> si_pid);
    }

    requests_it++;

    // Sends request approvals to all processes that have sent their request if the threshold is just reached
    if(requests_it == requests_threshold)
        for(int i = 0; i < requests_it; i++)
            send_single_approval(received_requests[i]);

    // Automaticly approves child's request if the threshold has beed reached before
    if(requests_it > requests_threshold)
        send_single_approval(info -> si_pid);
}


// Function used for handling SIGRT (makes use of siginfo structure, should be installed via sigaction with SA_SIGINFO flag enabled)
// Note that this handler is used to handle all real-time signals, so it should block every RT signal to prevent it from corrupting received signals related data
void sig_rt(int signo, siginfo_t *info, void *ucontext) {

    if(track_rtsignals == 0)
        return;

    // Saves info about process that sent the signal
    received_rtsignals[received_it].pid = info -> si_pid;
    received_rtsignals[received_it].signal = signo;
    received_it++;

    fprintf(stdout, MAGENTA "parent: received real-time signal %d from child process %d\n" RESET, signo, info -> si_pid);
}


// Prints example program usage
void print_usage() {
    printf("example usage: ./prog N M\n");
}


int main(int argc, char** argv) {

    if(argc < 3) {
        print_usage();
        exit(1);
    }

    children_number = atoi(argv[1]);
    requests_threshold = atoi(argv[2]);

    all_children_finished = 0;

    // Allocating memory for arrays storing tracked data

    // This array should always be created becaouse of the need to store all the children pids
    created_processes = malloc(sizeof(pid_t) * children_number);
    created_it = 0;

    if(track_requests) {
        received_requests = malloc(sizeof(pid_t) * children_number);
        requests_it = 0;
    }

    if(track_approvals) {
        sent_approvals = malloc(sizeof(pid_t) * children_number);
        approvals_it = 0;
    }

    if(track_rtsignals) {
        received_rtsignals = malloc(sizeof(struct received_signal) * children_number);
        received_it = 0;
    }

    // Another array that is always needed (to check which processes have already terminated in case of SIGINT caused main program termination)
    exit_codes = malloc(sizeof(struct exit_code) * children_number);
    exit_it = 0;

    // struct sigaction setup
    struct sigaction s_act;
    s_act.sa_handler = sig_int;
    sigfillset(&s_act.sa_mask);

    // Setting handler for SIGINT
    if(sigaction(SIGINT, &s_act, NULL) == -1) {

        perror("Cannot set SIGINT signal handler");
        exit(1);
    }

    // struct sigaction setup (blocks all signals except SIGINT, uses SA_NOCLDSTOP flag)
    s_act.sa_handler = sig_chld;
    sigdelset(&s_act.sa_mask, SIGINT);
    s_act.sa_flags = SA_NOCLDSTOP;

    // Setting handler for SIGCHLD
    if(sigaction(SIGCHLD, &s_act, NULL) == -1) {

        perror("Cannot set SIGCHLD signal handler");
        exit(1);
    }

    // struct sigaction setup (blocks all signals except SIGINT, uses SA_SIGINFO flag)
    s_act.sa_sigaction = sig_req;
    s_act.sa_flags = SA_SIGINFO;

    // Setting handler for REQUEST_SIGNAL
    if(sigaction(REQUEST_SIGNAL, &s_act, NULL) == -1) {

        perror("Cannot set SIGUSR1 signal handler");
        exit(1);
    }


    // struct sigaction setup (blocks all signals except SIGINT, uses SA_SIGINFO flag)
    s_act.sa_sigaction = sig_rt;
    s_act.sa_flags = SA_SIGINFO;

    // Setting handler for all SIGRT signals (except for SIGRTMAX)
    for(int i = SIGRTMIN; i < SIGRTMAX; i++)
        if(sigaction(i, &s_act, NULL) == -1) {

            perror("Cannot set SIGRT signal handler");
            exit(1);
        }

    fprintf(stdout, "handlers setup completed\n");

    sigset_t chld_mask, old_mask;
    sigfillset(&chld_mask);
    sigdelset(&chld_mask, SIGINT);

    if(sigprocmask(SIG_BLOCK, &chld_mask, &old_mask) == -1) {

        perror("Could not restore previous sigmask");
        exit(1);
    }

    for(int i = 0; i < children_number; i++)
        create_child();

    // Main loop of program execution
    while(all_children_finished == 0)
        sigsuspend(&old_mask);


    if(track_created == 1) {

        printf("created processes:\n");
        for(int i = 0; i < created_it; i++)
            printf("%d\n", created_processes[i]);

        printf("\n");
    }

    if(track_requests == 1) {

        printf("received requests children ids:\n");
        for(int i = 0; i < requests_it; i++)
            printf("%d\n", received_requests[i]);

        printf("\n");
    }

    if(track_approvals == 1) {

        printf("sent approvals children ids:\n");
        for(int i = 0; i < approvals_it; i++)
            printf("%d\n", sent_approvals[i]);

        printf("\n");
    }

    if(track_rtsignals == 1) {

        printf("received real-time signals:\n");
        printf("pid\tsigno\n");
        for(int i = 0; i < received_it; i++)
            printf("%d\t%d\n", received_rtsignals[i].pid, received_rtsignals[i].signal);

        printf("\n");
    }

    if(track_exit_codes == 1) {

        printf("children processes exit codes:\n");
        printf("pid\texit_code\n");
        for(int i = 0; i < exit_it; i++)
            printf("%d\t%d\n", exit_codes[i].pid, exit_codes[i].code);

        printf("\n");
    }
}
