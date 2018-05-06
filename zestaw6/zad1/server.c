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
#include <sys/ipc.h>
#include <sys/msg.h>
#include "server_info.h"

int server_queue_id;
int end_request_received;
int rejected_client_queue_id;

struct client {
    int pid;
    int queue_id;
    int id;
};

struct client clients[SI_CLIENTS_LIMIT];

struct string_message {

    long type;
    char message[SI_MESSAGE_MAX_SIZE];
};


// Sends message to given queue
int send_string_message(int client_pid, long type, const char *message) {

    int q_id;

    if(client_pid == -1)
        q_id = rejected_client_queue_id;

    else {

        int client_slot;
        for(client_slot = 0; client_slot < SI_CLIENTS_LIMIT; client_slot++)
            if(clients[client_slot].pid == client_pid)
                break;

        if(client_slot == SI_CLIENTS_LIMIT)
            return -1;

        q_id = clients[client_slot].queue_id;
    }

    struct string_message msg;
    msg.type = type;
    strcpy(msg.message, message);

    if(msgsnd(q_id, &msg, SI_MESSAGE_MAX_SIZE, 0) == -1) {
        fprintf(stderr, "Could not successfully send message \"%s\": ", message);
        perror("");
    }

    return 0;
}


// ==================================================================================================
// ALL TASKS PERFORMED BY SERVER
// ==================================================================================================

// Processes string that contains CALC request message
int calc_task(char *task, char *ans) {

    // CALC expects exactly 3 tokens
    char *token_ptr;
    char *tokens[3];

    token_ptr = strtok(task, " \t\n");
    int client_pid = atoi(token_ptr);

    for(int i = 0; i < 3; i++) {
        token_ptr = strtok(NULL, " \t\n");

        if(token_ptr == NULL) {
            sprintf(ans, "invalid operation");
            return client_pid;
        }

        tokens[i] = token_ptr;
    }

    if(strtok(NULL, " \n") != NULL) {
        sprintf(ans, "invalid operation");
        return client_pid;
    }

    int a = atoi(tokens[1]);
    int b = atoi(tokens[2]);
    int result;

    if(strcmp(tokens[0], "ADD") == 0)
        result = a + b;

    else if(strcmp(tokens[0], "SUB") == 0)
        result = a - b;

    else if(strcmp(tokens[0], "MUL") == 0)
        result = a * b;

    else if(strcmp(tokens[0], "DIV") == 0)
        result = a / b;

    else {
        sprintf(ans, "invalid operation");
        return client_pid;
    }

    sprintf(ans, "%d", result);
    return client_pid;
}


// Processes string that contains MIRROR request message
int mirror_task(char *task, char *ans) {

    char *token_ptr = strtok(task, " \t\n");

    if(token_ptr == NULL)
        return -1;

    int client_pid = atoi(token_ptr);

    token_ptr = strtok(NULL, "\0");

    int iter = 0;
    while(token_ptr[iter] != '\0')
        iter++;

    for(int i = 0; i < iter; i++)
        ans[i] = token_ptr[iter-i-1];

    ans[iter] = '\0';
    return client_pid;
}


// Processes string that contains TIME request message
int time_task(char *task, char *ans) {

    char *token_ptr = strtok(task, " \t\n");

    if(token_ptr == NULL)
        return -1;

    int client_pid = atoi(token_ptr);

    time_t t = time(NULL);
    struct tm *time = localtime(&t);
    strftime(ans, SI_MESSAGE_MAX_SIZE, "%c", time);

    return client_pid;
}


// Opens new client's message queue
int register_new_client(char *request, char *ans) {

    // Searching for free slot in clients array
    int free_place = 0;
    while(free_place < SI_CLIENTS_LIMIT) {
        if(clients[free_place].id == -1)
            break;

        free_place++;
    }

    char *token_ptr = strtok(request, " \t\n");
    if(token_ptr == NULL)
        return -1;

    int new_client_pid = atoi(token_ptr);

    token_ptr = strtok(NULL, " \t\n");
    if(token_ptr == NULL)
        return -1;

    int queue_id = atoi(token_ptr);

    if(free_place == SI_CLIENTS_LIMIT) {
        sprintf(ans, "No more free slots");
        rejected_client_queue_id = queue_id;
        return -3;
    }

    clients[free_place].pid = new_client_pid;
    clients[free_place].id = free_place;
    clients[free_place].queue_id = queue_id;

    sprintf(ans, "Successfully registered with id %d", free_place);
    return new_client_pid;
}


int remove_client(char *request) {

    char *token_ptr = strtok(request, " \t\n");

    if(token_ptr == NULL)
        return -1;

    int client_pid = atoi(token_ptr);

    int client_slot;
    for(client_slot = 0; client_slot < SI_CLIENTS_LIMIT; client_slot++)
        if(clients[client_slot].pid == client_pid)
            break;

    if(client_slot == SI_CLIENTS_LIMIT)
        return -1;

    fprintf(stdout, "removed client %d, pid: %d\n", clients[client_slot].id, clients[client_slot].pid);

    clients[client_slot].id = -1;
    clients[client_slot].pid = -1;
    return 0;
}

// ==================================================================================================
// ==================================================================================================
// ==================================================================================================


// Creates server queue at the beginning of the program
void create_server_queue() {

    char *home = getenv("HOME");

    int server_queue_key;

    if((server_queue_key = ftok(home, SI_IPC_KEY_NUMBER)) == -1) {
        perror("Error occurred while using ftok");
        exit(1);
    }

    if((server_queue_id = msgget(server_queue_key, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR )) == -1) {
        perror("Error occurred while creating server queue");
        exit(1);
    }
}


// Waits for request and send response
void process_request() {

    struct string_message msg;

    if(msgrcv(server_queue_id, &msg, SI_MESSAGE_MAX_SIZE, (SI_REQ_END - 1) * (-1), IPC_NOWAIT) == -1)
        msgrcv(server_queue_id, &msg, SI_MESSAGE_MAX_SIZE, 0, 0);

    fprintf(stdout, "received message type %ld: %s\n", msg.type, msg.message);

    int client_pid;
    char ans[SI_MESSAGE_MAX_SIZE];

    if(msg.type == SI_REQ_REGISTER)
        client_pid = register_new_client(msg.message, ans);

    else if(msg.type == SI_REQ_TIME)
        client_pid = time_task(msg.message, ans);

    else if(msg.type == SI_REQ_CALC)
        client_pid = calc_task(msg.message, ans);

    else if(msg.type == SI_REQ_MIRROR)
        client_pid = mirror_task(msg.message, ans);

    else if(msg.type == SI_REQ_STOP) {
        remove_client(msg.message);
        client_pid = -2;
    }

    else if(msg.type == SI_REQ_END)
        end_request_received = 1;


    if(client_pid == -1)
        fprintf(stdout, "unknown client's pid\n");

    else if(client_pid == -3) {
        send_string_message(-1, SI_REJECTED, ans);
        fprintf(stdout, "Cannot register new client - no more free slots\n");
    }

    else if(client_pid > 0)
        send_string_message(client_pid, SI_ACCEPTED, ans);

}



// Function used when exiting from program - deletes server queue
static void exit_fun() {

    if(server_queue_id != -1) {

        if(msgctl(server_queue_id, IPC_RMID, NULL) == -1)
            perror("Error occurred while deleting server queue");
    }
}

// Function used for handling SIGINT
void sig_int(int signo) {

    fprintf(stderr, "SIGINT received - closing server\n");
    exit(1);
}


int main(int argc, char** argv) {

    server_queue_id = -1;
    end_request_received = 0;

    for(int i = 0; i < SI_CLIENTS_LIMIT; i++) {
        clients[i].id = -1;
        clients[i].pid = -1;
    }

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

    create_server_queue();

    while(end_request_received == 0)
        process_request();

}
