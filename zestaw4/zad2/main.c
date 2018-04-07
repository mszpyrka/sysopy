#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

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

    request_approved = 1;
}

void ch_sig_quit(int signo) {

    exit(22);
}

// Function executed by every child process
void child_proc_fun(int seed) {

    request_approved = 0;

    // Ignores interrupt signal in order not to end the process in case of interactive terminal interrupt
    // struct sigaction setup
    struct sigaction act_int;
    act_int.sa_handler = SIG_IGN;
    sigemptyset(&act_int.sa_mask);

    // Setting handler for SIGINT
    if(sigaction(SIGINT, &act_int, NULL) == -1) {

        perror("Cannot set SIGINT signal handler for child process");
        exit(20);
    }

    // struct sigaction setup
    struct sigaction act_usr2;
    act_usr2.sa_handler = ch_sig_usr2;
    sigemptyset(&act_usr2.sa_mask);

    // Setting handler for SIGUSR2
    if(sigaction(SIGUSR2, &act_usr2, NULL) == -1) {

        perror("Cannot set SIGUSR2 signal handler for child process");
        exit(20);
    }

    // struct sigaction setup
    struct sigaction act_quit;
    act_quit.sa_handler = ch_sig_quit;
    sigemptyset(&act_quit.sa_mask);

    // Setting handler for SIGUSR2
    if(sigaction(SIGQUIT, &act_quit, NULL) == -1) {

        perror("Cannot set SIGQUIT signal handler for child process");
        exit(20);
    }

    srand(seed);
    int sleep_time = rand() % 11; // Randomly chooses number between 0 and 10

    sleep(sleep_time);

    union sigval none;

    sigset_t usr2_mask, old_mask;
    sigemptyset(&usr2_mask);
    sigaddset(&usr2_mask, SIGUSR2);

    // Blocks SIGUSR2 for critical part of code
    sigprocmask(SIG_BLOCK, &usr2_mask, &old_mask);

    // Sends request to parent process
    if(sigqueue(getppid(), SIGUSR1, none) == -1) {

        fprintf(stderr, "Could not send SIGUSR1 signal to parent process %d", getppid());
        perror("");
        exit(20);
    }

    fprintf(stderr, "ja, %d, wyslalem sygnal do taty %d\n", getpid(), getppid());

    // Unblocks SIGUSR2
    while(request_approved == 0)
        sigsuspend(&old_mask);

    // Restores old sigmask
    sigprocmask(SIG_SETMASK, &old_mask, NULL);

    // Sends random real-time signal to parent process
    int rand_sig = SIGRTMIN + rand() % (SIGRTMAX - SIGRTMIN + 1);

    if(sigqueue(getppid(), rand_sig, none) == -1) {

        fprintf(stderr, "Could not send SIGRT signal to parent process %d", getppid());
        perror("");
        exit(20);
    }

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
void send_approval(pid_t child_pid) {

    kill(child_pid, SIGUSR2);

    if(track_approvals) {

        sent_approvals[approvals_it] = child_pid;
        approvals_it++;
    }
}


// Function used for handling SIGINT
void sig_int(int signo) {

    printf("Received signal SIGINT\n");

    for(int i = 0; i < created_it; i++)
        kill_child(created_processes[i]);

    exit(0);
}


// Function used for handling SIGCHLD
// This handler assumes that it is being called only after child process termination, thus should be installed with SA_NOCHLDSTOP flag
void sig_chld(int signo) {

    int status;
    int child_id;

    // Saves info about process that sent the signal
    while((child_id = waitpid(-1, &status, WNOHANG)) > 0) {
        exit_codes[exit_it].pid = child_id;
        exit_codes[exit_it].code = WEXITSTATUS(status);
        exit_it++;
    }

    // Exits if all children have finished their processes and all signals have beed processed successfully
    if(exit_it == children_number)
        all_children_finished = 1;
}


// Function used for handling SIGUSR1 (makes use of siginfo structure, should be installed via sigaction with SA_SIGINFO flag enabled)
// This handler sends approvals to all processes that sent their request when the threshold is reached, so should block all signals that modify received request data
void sig_usr1(int signo, siginfo_t *info, void *ucontext) {

    if(track_requests == 1)
        received_requests[requests_it] = info -> si_pid;

    requests_it++;

    // Sends request approvals to all processes that have sent their request if the threshold is just reached
    if(requests_it == requests_threshold)
        for(int i = 0; i < requests_it; i++)
            send_approval(received_requests[i]);

    // Automaticly approves child's request if the threshold has beed reached before
    if(requests_it > requests_threshold)
        send_approval(info -> si_pid);
}


// Function used for handling SIGRT (makes use of siginfo structure, should be installed via sigaction with SA_SIGINFO flag enabled)
// Note that this handler is used to handle all real-time signals, so it should block every RT signal to prevent it from corrupting received signals related data
void sig_rt(int signo, siginfo_t *info, void *ucontext) {

    printf("received signal %d from process %d\n", signo, info -> si_pid);
    if(track_rtsignals == 0)
        return;

    // Saves info about process that sent the signal
    received_rtsignals[received_it].pid = info -> si_pid;
    received_rtsignals[received_it].signal = signo;
    received_it++;
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

    // Another array that is necessary to be able to store all processes that have already exited
    exit_codes = malloc(sizeof(struct exit_code) * children_number);
    exit_it = 0;

    // struct sigaction setup
    struct sigaction act_int;
    act_int.sa_handler = sig_int;
    sigemptyset(&act_int.sa_mask);

    // Setting handler for SIGINT
    if(sigaction(SIGINT, &act_int, NULL) == -1) {

        perror("Cannot set SIGINT signal handler");
        exit(1);
    }

    // struct sigaction setup
    struct sigaction act_chld;
    act_chld.sa_handler = sig_chld;
    sigemptyset(&act_chld.sa_mask);
    act_chld.sa_flags = SA_NOCLDSTOP;

    // Setting handler for SIGCHLD
    if(sigaction(SIGCHLD, &act_chld, NULL) == -1) {

        perror("Cannot set SIGCHLD signal handler");
        exit(1);
    }

    // struct sigaction setup
    struct sigaction act_usr1;
    act_usr1.sa_sigaction = sig_usr1;
    sigemptyset(&(act_usr1.sa_mask));
    sigaddset(&(act_usr1.sa_mask), SIGUSR1);
    act_usr1.sa_flags = SA_SIGINFO;

    // Setting handler for SIGUSR1
    if(sigaction(SIGUSR1, &act_usr1, NULL) == -1) {

        perror("Cannot set SIGUSR1 signal handler");
        exit(1);
    }


    // struct sigaction setup
    struct sigaction act_rt;
    act_rt.sa_sigaction = sig_rt;

    // Adding all RT signals blocking to handler's mask
    sigemptyset(&(act_rt.sa_mask));
    for(int i = SIGRTMIN; i <= SIGRTMAX; i++)
        sigaddset(&(act_rt.sa_mask), i);

    act_rt.sa_flags = SA_SIGINFO;

    // Setting handler for SIGRT
    for(int i = SIGRTMIN; i <= SIGRTMAX; i++)
        if(sigaction(i, &act_rt, NULL) == -1) {

            perror("Cannot set SIGRT signal handler");
            exit(1);
        }

    printf("handlers setup completed\n");

    sigset_t chld_mask, old_mask;
    sigemptyset(&chld_mask);
    sigaddset(&chld_mask, SIGCHLD);

    sigprocmask(SIG_BLOCK, &chld_mask, &old_mask);

    for(int i = 0; i < children_number; i++)
        create_child();

    while(all_children_finished == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);

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
