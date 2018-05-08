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

int invited;

// Opens all necessery IPC structures
void open_barbershop() {

    // Semaphores setup
    sem_wtroom_ptr = sem_open(SEM_WTROOM_NAME, 0);
    sem_chair_ptr = sem_open(SEM_CHAIR_NAME, 0);
    sem_shaving_ptr = sem_open(SEM_SHAVING_NAME, 0);
    sem_shvroom_ptr = sem_open(SEM_SHVROOM_NAME, 0);
    //sem_task_done_ptr = sem_open(SEM_TASK_DONE_NAME, 0);

    if(sem_wtroom_ptr == SEM_FAILED || sem_chair_ptr == SEM_FAILED
        || sem_shaving_ptr == SEM_FAILED || sem_shvroom_ptr == SEM_FAILED) {
        perror("Error occurred while opening semaphores");
        exit(1);
    }

    // Shared memory segment setup
    mem_barbershop_fd = shm_open(MEM_BRBSHOP_NAME, O_RDWR, 0);
    shared_barbershop = (struct barbershop*) mmap(NULL, sizeof(struct barbershop), PROT_READ | PROT_WRITE, MAP_SHARED,
                                            mem_barbershop_fd, 0);

    if(shared_barbershop == (void*)-1) {
        perror("Could not attach shared memory segment");
        exit(1);
    }
}

// Sends SIG_WAKEUP to the barber's process
void wake_barber_up() {

    print_time_message("waking up the barber\n");
    kill(shared_barbershop -> barber_pid, SIG_WAKEUP);
}

// Suspends client's process until SIG_INVITE is received
void wait_in_queue() {

    enqueue(&(shared_barbershop -> clients_queue), getpid());   // Adds process id to the queue
    shared_barbershop -> empty_sits_number--;

    sigset_t tmp_mask, old_mask;
    sigemptyset(&tmp_mask);
    sigaddset(&tmp_mask, SIG_INVITE);
    sigprocmask(SIG_BLOCK, &tmp_mask, &old_mask); // Temporarily blocks SIG_INVITE

    print_time_message("taking a sit in the waiting room\n");

    invited = 0;
    sem_post(sem_wtroom_ptr);                // Unlocks waiting room

    while(invited == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);  // Restores previous mask
}

// Waits for the access to shaving chair
void take_a_sit_on_shaving_chair() {

    sem_wait(sem_shvroom_ptr);   // Waits for the access to the shaving room
    print_time_message("taking a sit on the shaving shair\n");
    shared_barbershop -> shaved_client_pid = getpid();
    sem_post(sem_chair_ptr);    // Signalizes having taken a sit on the chair
}

// Controls the process of being shaved
void shave_beard_and_mustache() {

    sem_wait(sem_shaving_ptr);  // Waits until the shaving ends
    print_time_message("leaving the barbershop with nicely groomed mustache\n");
}

// Tries to get shaved, returns 1 on success, 0 otherwise
int enter_barbershop() {

    sem_wait(sem_wtroom_ptr);       // Obtaining exclusive access to the waiting room

    if(shared_barbershop -> is_sleeping == 1) {
        wake_barber_up();
        take_a_sit_on_shaving_chair();
        sem_post(sem_wtroom_ptr);   // Unlocking the access to the waiting room
        shave_beard_and_mustache();
    }

    else {

        if(shared_barbershop -> empty_sits_number > 0) {
            wait_in_queue();        // sem_wtroom is being unlocked inside this procedure
            take_a_sit_on_shaving_chair();
            shave_beard_and_mustache();
        }

        else {
            print_time_message("no places in the waiting room, I'm leaving\n");
            sem_post(sem_wtroom_ptr);       // Unlocking the access to the waiting room
            return 0;
        }
    }

    return 1;
}

// Sends the client's process to the barbershop until number of shavings reaches given limit
void get_shaved_multiple_times(int shavings_number) {

    int shavings_received = 0;

    while(shavings_received < shavings_number)
        shavings_received += enter_barbershop();
}

// Function used for handling SIG_INVITE
void sig_invite(int signo) {

    invited = 1;
    print_time_message("SIG_INVITE received - going to the shaving room\n");
}

void print_usage() {

    printf("Example usage: ./client <clients_number> <shavings_number>\n");
}

int main(int argc, char** argv) {

    // struct sigaction setup
    struct sigaction act_invite;
    act_invite.sa_handler = sig_invite;
    sigemptyset(&act_invite.sa_mask);
    act_invite.sa_flags = 0;

    // Setting handler for SIG_INVITE
    if(sigaction(SIG_INVITE, &act_invite, NULL) == -1) {

        perror("Cannot set SIG_INVITE signal handler");
        exit(1);
    }

    if(argc != 3) {
        printf("Wrong arguments format\n");
        print_usage();
        exit(1);
    }

    int clients_number = atoi(argv[1]);
    int shavings_number = atoi(argv[2]);

    open_barbershop();

    for(int i = 0; i < clients_number; i++) {
        pid_t id = fork();

        if(id == 0) {
            get_shaved_multiple_times(shavings_number);
            return 0;
        }
    }

    for(int i = 0; i < clients_number; i++)
        wait(NULL);
}
