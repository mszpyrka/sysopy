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

key_t client_queue_id[SI_CLIENTS_LIMIT];
int server_queue_id;

int client_pid[SI_CLIENTS_LIMIT];


struct string_message {

    long type;
    char message[SI_MESSAGE_MAX_SIZE];
};


// Processes string that contains CALC request message
int calc_task(char *task, char *ans) {

    // CALC expects exactly 3 tokens
    char *token_ptr;
    char *tokens[3];

    token_ptr = strtok(task, " \t");
    int client_id = atoi(token_ptr);

    for(int i = 0; i < 3; i++) {
        token_ptr = strtok(NULL, " \t");

        if(token_ptr == NULL)
            return -1;

        tokens[i] = token_ptr;
    }

    if(strtok(NULL, " \n") != NULL)
        return -1;

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

    else
        return -1;

    sprintf(ans, "%d", result);
    return client_id;
}


// Processes string that contains MIRROR request message
int mirror_task(char *task, char *ans) {

    char *token_ptr = strtok(task, " \t");

    if(token_ptr == NULL)
        return -1;

    int client_id = atoi(token_ptr);

    token_ptr = strtok(NULL, "\0");

    int iter = 0;
    while(token_ptr[iter] != '\0')
        iter++;

    for(int i = 0; i < iter; i++)
        ans[i] = token_ptr[iter-i-1];

    ans[iter] = '\0';
    return client_id;
}


// Processes string that contains TIME request message
int time_task(char *task, char *ans) {

    char *token_ptr = strtok(task, " \t");

    if(token_ptr == NULL)
        return -1;

    int client_id = atoi(token_ptr);

    time_t t = time(NULL);
    struct tm *time = localtime(&t);
    strftime(ans, SI_MESSAGE_MAX_SIZE, "%c", time);

    return client_id;
}


void process_message(struct string_message *message) {
    //TODO
}


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


// Sends message to given queue
void send_string_message(int client_id, long type, const char *message) {

    struct string_message msg;
    msg.type = type;
    strcpy(msg.message, message);

    //TODO
    //if(msgsnd(queue_id, &msg, sizeof(struct string_message), ))
}

// Function used when exiting from program - deletes server queue
static void exit_fun() {

    if(server_queue_id != -1) {

        if(msgctl(server_queue_id, IPC_RMID, NULL) == -1)
            perror("Error occurred while deleting server queue");
    }
}


int main(int argc, char** argv) {

    server_queue_id = -1;

    for(int i = 0; i < SI_CLIENTS_LIMIT; i++)
        client_queue_id[i] = -1;

    if (atexit(exit_fun) != 0) {

        perror("Cannot set atexit function");
        exit(1);
    }

    create_server_queue();

    char *buffer1 = malloc(30);
    char *buffer2 = malloc(30);

    sprintf(buffer1, "2341 DIV 16 -16");
    int id;
    if((id = calc_task(buffer1, buffer2)) == -1)
        printf("no i hui\n");

    else
        printf("%d: %s\n", id, buffer2);

    sprintf(buffer1, "2134 reverse string");
    mirror_task(buffer1, buffer2);
    printf("%s\n", buffer2);

    time_task(buffer1, buffer2);
    printf("%s\n", buffer2);

}
