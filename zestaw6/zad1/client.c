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

int client_queue_id;
int server_queue_id;

struct string_message {

    long type;
    char message[SI_MESSAGE_MAX_SIZE];
};



// Sends message to given queue
void send_string_message(long type, const char *message) {

    struct string_message msg;
    msg.type = type;
    sprintf(msg.message, "%d %s", getpid(), message);

    if(msgsnd(server_queue_id, &msg, SI_MESSAGE_MAX_SIZE, 0) == -1) {
        printf("Could not successfully send message \"%s\": ", message);
        perror("");
    }
}

int receive_string_message(char *buffer) {

    struct string_message msg;

    if(msgrcv(client_queue_id, &msg, SI_MESSAGE_MAX_SIZE, 0, 0) == -1) {
        printf("Could not successfully read message \"%s\": ", buffer);
        perror("");
        return -1;
    }

    strcpy(buffer, msg.message);
    return msg.type;
}


// "Connects" to the server queue at the beginning of the program
void obtain_server_queue() {

    char *home = getenv("HOME");

    int server_queue_key;

    if((server_queue_key = ftok(home, SI_IPC_KEY_NUMBER)) == -1) {
        perror("Error occurred while using ftok");
        exit(1);
    }

    if((server_queue_id = msgget(server_queue_key, 0 )) == -1) {
        perror("Error occurred while searching for server queue");
        exit(1);
    }
}


// Creates private queue for server -> client communication
void create_client_queue() {

    if((client_queue_id = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR)) == -1) {
        perror("Error occurred while creating client queue");
        exit(1);
    }
}


// Sends registration request to server
void connect_to_server() {
    char buffer[SI_MESSAGE_MAX_SIZE];
    sprintf(buffer, "%d", client_queue_id);
    send_string_message(SI_REQ_REGISTER, buffer);
    int response = receive_string_message(buffer);

    if(response == SI_REJECTED) {
        fprintf(stderr, "Registration request rejected by server!\n");
        exit(1);
    }

    fprintf(stderr, "%s\n", buffer);
}

// Function used when exiting from program - deletes server queue
static void exit_fun() {

    if(client_queue_id != -1) {

        if(server_queue_id != -1)
            send_string_message(SI_REQ_STOP, "");

        if(msgctl(client_queue_id, IPC_RMID, NULL) == -1)
            perror("Error occurred while deleting client queue");
    }
}


// Function used for handling SIGINT
void sig_int(int signo) {

    fprintf(stderr, "SIGINT received - closing connection\n");
    exit(1);
}


void process_next_line(char *buffer) {

    char *type = strtok(buffer, " \t\n");
    if(type == NULL)   // empty line
        return;

    char *message = strtok(NULL, "\n");
    int message_type;

    if(strcmp(type, "CALC") == 0)
        message_type = SI_REQ_CALC;

    else if(strcmp(type, "TIME") == 0)
        message_type = SI_REQ_TIME;

    else if(strcmp(type, "MIRROR") == 0)
        message_type = SI_REQ_MIRROR;

    else if(strcmp(type, "END") == 0)
        message_type = SI_REQ_END;

    else if(strcmp(type, "STOP") == 0)
        exit(0);

    else {
        printf("Invalid message type: %s\n", type);
        return;
    }

    send_string_message(message_type, message);

    if(message_type == SI_REQ_END)
        exit(0);

    receive_string_message(buffer);
    printf("%s\n", buffer);
}


int main(int argc, char** argv) {

    server_queue_id = -1;
    client_queue_id = -1;

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

    obtain_server_queue();
    create_client_queue();
    connect_to_server();

    char* buffer = NULL;
    size_t buffer_size = 0;
    size_t bytes_read;

    while((bytes_read = getline(&buffer, &buffer_size, stdin)) != -1)
        process_next_line(buffer);
}
