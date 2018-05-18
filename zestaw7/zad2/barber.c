#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "general.h"

int mem_barbershop_fd;
sem_t *sem_wtroom_ptr;
sem_t *sem_chair_ptr;
sem_t *sem_shaving_ptr;
sem_t *sem_shvroom_ptr;
sem_t *sem_task_done_ptr;
struct barbershop *shared_barbershop;
int should_wake_up;



// Creates and initializes all necessery IPC structures
// All semaphores are initially set to 0
void initialize_barbershop(int waiting_room_size) {

    // Semaphores setup
    sem_wtroom_ptr = sem_open(SEM_WTROOM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    sem_chair_ptr = sem_open(SEM_CHAIR_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    sem_shaving_ptr = sem_open(SEM_SHAVING_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    sem_shvroom_ptr = sem_open(SEM_SHVROOM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);

    // Shared memory segment setup
    mem_barbershop_fd = shm_open(MEM_BRBSHOP_NAME, O_CREAT | O_EXCL | O_RDWR, S_IWUSR | S_IRUSR);
    ftruncate(mem_barbershop_fd, sizeof(struct barbershop));


    shared_barbershop = (struct barbershop*) mmap(NULL, sizeof(struct barbershop), PROT_READ | PROT_WRITE, MAP_SHARED,
                                            mem_barbershop_fd, 0);

    if(shared_barbershop == (void*)-1) {
        perror("Could not attach shared memory segment");
        exit(1);
    }


    shared_barbershop -> all_sits_number = waiting_room_size;
    shared_barbershop -> empty_sits_number = waiting_room_size;
    init_queue(&(shared_barbershop -> clients_queue), waiting_room_size);
    shared_barbershop -> barber_pid = getpid();
    shared_barbershop -> is_sleeping = 0;
    shared_barbershop -> shaved_client_pid = -1;
}

// Sets all necessary values and suspends execution waiting for SIG_WAKEUP
// Assumes that barber's process is currently locking waiting room semaphore
void fall_asleep() {

    shared_barbershop -> is_sleeping = 1;

    sigset_t tmp_mask, old_mask;
    sigemptyset(&tmp_mask);
    sigaddset(&tmp_mask, SIG_WAKEUP);
    sigprocmask(SIG_BLOCK, &tmp_mask, &old_mask); // Temporarily blocks SIG_WAKEUP

    print_time_message("falling asleep\n");
    should_wake_up = 0;
    sem_post(sem_wtroom_ptr);                // Unlocks waiting room

    while(should_wake_up == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);  // Restores previous mask
    shared_barbershop -> is_sleeping = 0;
}

// Performs all operations related to the process of shaving client
void shave_client() {

    sem_post(sem_shvroom_ptr);       // Unlocks access to the shaving room
    sem_wait(sem_chair_ptr);         // Blocks until client takes a sit on the chair

    print_time_message("");         // Begins shaving
    fprintf(stdout, "starting shaving client %d\n", shared_barbershop -> shaved_client_pid);

    print_time_message("");         // Ends shaving
    fprintf(stdout, "finishing shaving client %d\n", shared_barbershop -> shaved_client_pid);

    sem_post(sem_shaving_ptr);      // Signalizes the ending of the shaving
}

// Checks whether there are any clients waiting, if so - invites the first one to the shaving room (and returns 1), if not - returns 0
int invite_next_client() {

    sem_wait(sem_wtroom_ptr);   // Locks access to the waiting room

    pid_t next_client_pid = dequeue(&(shared_barbershop -> clients_queue));

    if(next_client_pid == -1)   // No clients waiting
        return 0;

    shared_barbershop -> empty_sits_number++;
    print_time_message("");
    fprintf(stdout, "inviting client %d from the waiting room\n", next_client_pid);
    kill(next_client_pid, SIG_INVITE);      // Sends invitation signal to client's process
    sem_post(sem_wtroom_ptr);               // Unlocks the waiting room
    return 1;
}

// Function used when exiting from program - deletes all created IPC structures
static void exit_fun() {

    if(mem_barbershop_fd != -1) {

        if(munmap(shared_barbershop, sizeof(struct barbershop)))
            perror("Error occurred while unmapping barbershop shared memory segment");

        if(shm_unlink(MEM_BRBSHOP_NAME) == -1)
            perror("Error occurred while unlinking barbershop shared memory segment");
    }

    if(sem_wtroom_ptr != (sem_t*)-1) {

        if(sem_close(sem_wtroom_ptr) == -1)
            perror("Error occurred while deleting waiting room semaphore");

        if(sem_unlink(SEM_WTROOM_NAME) == -1)
            perror("Error occurred while unlinking waiting room semaphore");
    }

    if(sem_chair_ptr != (sem_t*)-1) {

        if(sem_close(sem_chair_ptr) == -1)
            perror("Error occurred while deleting chair semaphore");

        if(sem_unlink(SEM_CHAIR_NAME) == -1)
            perror("Error occurred while unlinking chair semaphore");
    }

    if(sem_shaving_ptr != (sem_t*)-1) {

        if(sem_close(sem_shaving_ptr) == -1)
            perror("Error occurred while deleting shaving semaphore");

        if(sem_unlink(SEM_SHAVING_NAME) == -1)
            perror("Error occurred while unlinking shaving semaphore");
    }

    if(sem_shvroom_ptr != (sem_t*)-1) {

        if(sem_close(sem_shvroom_ptr) == -1)
            perror("Error occurred while deleting shaving room semaphore");

        if(sem_unlink(SEM_SHVROOM_NAME) == -1)
            perror("Error occurred while unlinking shaving room semaphore");
    }
}

// Function used for handling SIGINT
void sig_int(int signo) {

    fprintf(stderr, "SIGINT received - barbershop work interrupted\n");
    exit(1);
}

// Function used for handling SIGINT
void sig_term(int signo) {

    fprintf(stderr, "SIGTERM received - closing barbershop\n");
    exit(0);
}

// Function used for handling SIG_WAKEUP
void sig_wakeup(int signo) {

    should_wake_up = 1;
    print_time_message("SIG_WAKEUP received - going back to work\n");
}


void print_usage() {

    printf("Example usage: ./barber <waiting_room_size>\n");
}

int main(int argc, char** argv) {

    mem_barbershop_fd = -1;
    sem_wtroom_ptr = (sem_t*)-1;
    sem_chair_ptr = (sem_t*)-1;
    sem_shaving_ptr = (sem_t*)-1;
    sem_shvroom_ptr = (sem_t*)-1;
    sem_task_done_ptr = (sem_t*)-1;
    should_wake_up = 0;

    if (atexit(exit_fun) != 0) {

        perror("Cannot set atexit function");
        exit(1);
    }

    // struct sigaction setup
    struct sigaction act_int;
    act_int.sa_handler = sig_int;
    sigemptyset(&act_int.sa_mask);
    act_int.sa_flags = 0;

    // Setting handler for SIGINT
    if(sigaction(SIGINT, &act_int, NULL) == -1) {

        perror("Cannot set SIGINT signal handler");
        exit(1);
    }

    // struct sigaction setup
    struct sigaction act_term;
    act_term.sa_handler = sig_term;
    sigemptyset(&act_term.sa_mask);
    act_term.sa_flags = 0;

    // Setting handler for SIGTERM
    if(sigaction(SIGTERM, &act_term, NULL) == -1) {

        perror("Cannot set SIGTERM signal handler");
        exit(1);
    }

    // struct sigaction setup
    struct sigaction act_wakeup;
    act_wakeup.sa_handler = sig_wakeup;
    sigemptyset(&act_wakeup.sa_mask);
    act_wakeup.sa_flags = 0;

    // Setting handler for SIG_WAKEUP
    if(sigaction(SIG_WAKEUP, &act_wakeup, NULL) == -1) {

        perror("Cannot set SIG_WAKEUP signal handler");
        exit(1);
    }

    if(argc != 2) {
        printf("Wrong arguments format\n");
        print_usage();
        exit(1);
    }

    int waiting_room_size = atoi(argv[1]);

    initialize_barbershop(waiting_room_size);

    while(1) {

        fall_asleep();
        shave_client();

        while(invite_next_client() == 1)
            shave_client();

    }
}
