#ifndef BARBER_GENERAL_H
#define BARBER_GENERAL_H

#include <semaphore.h>

#define CLIENTS_QUEUE_MAX_SIZE  128

#define SIG_WAKEUP  SIGUSR1     // Signal used to wake the barber up
#define SIG_INVITE  SIGUSR2     // Signal used to invite a client from waiting room to barber's room

// Key id's used to obtain IPC structures access (all processes should use $HOME variable for generating IPC keys)
#define SEM_WTROOM_NAME     "/sem_wtroom"  // Semaphore used to control access to waiting room
#define SEM_CHAIR_NAME      "/sem_chair"  // Semaphore used to control access to barber's chair
#define SEM_SHAVING_NAME    "/sem_shaving"  // Semaphore used to control shaving process of a single client
#define SEM_SHVROOM_NAME    "/sem_shvroom"
#define SEM_TASK_DONE_NAME  "/sem_task"     // Additional semaphore used to indicate that some process finished some task
#define MEM_BRBSHOP_NAME    "/mem_brbshop"  // Shared memory segment containing a barbershop structure

struct queue {

    int max_size;
    int current_size;
    pid_t clients[CLIENTS_QUEUE_MAX_SIZE];
    int front;
    int back;
};


struct barbershop {

    // Waiting room
    struct queue clients_queue;
    int all_sits_number;
    int empty_sits_number;

    // Barber's room
    pid_t barber_pid;
    pid_t shaved_client_pid;
    int is_sleeping;

};

void init_queue(struct queue *q, int size);
int enqueue(struct queue *q, pid_t client_pid);
pid_t dequeue(struct queue *q);

void print_time_message(const char *message);

#endif
