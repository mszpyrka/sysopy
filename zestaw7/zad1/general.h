#ifndef BARBER_GENERAL_H
#define BARBER_GENERAL_H

#define CLIENTS_QUEUE_MAX_SIZE  128

#define SIG_WAKEUP  SIGUSR1     // Signal used to wake the barber up
#define SIG_INVITE  SIGUSR2     // Signal used to invite a client from waiting room to barber's room

// Key id's used to obtain IPC structures access (all processes should use $HOME variable for generating IPC keys)
#define SEM_WTROOM_KEY_ID   27  // Semaphore used to control access to waiting room
#define SEM_CHAIR_KEY_ID    28  // Semaphore used to control access to barber's chair
#define SEM_SHAVING_KEY_ID  29  // Semaphore used to control shaving process of a single client
#define SEM_SHVROOM_KEY_ID  30
#define MEM_BRBSHOP_KEY_ID  31  // Shared memory segment containing a barbershop structure

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

int sem_decrease(int sem_id);
int sem_increase(int sem_id);
int sem_wait_for_zero(int sem_id);

#endif
