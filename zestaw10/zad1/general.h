#ifndef TASK10_GENERAL_H
#define TASK10_GENERAL_H

#include <arpa/inet.h>

// general settings:

#define SOCKET_PROTOCOL     SOCK_STREAM

// server settings: 

#define SERVER_NAME             "cluster_supervisor"
#define SERVER_CLIENTS_LIMIT    30
#define PING_INTERVAL           5
#define PING_TIMEOUT            1

// client settings:

#define CLIENT_MAX_NAME_LENGTH  32

// socket modes: 

#define MODE_INET       1
#define MODE_UNIX       2

// task types:

#define TASK_ADD    1
#define TASK_SUB    2
#define TASK_MUL    3
#define TASK_DIV    4


// client representation
struct client {

    char name[CLIENT_MAX_NAME_LENGTH + 1];
    int socket_fd;
    int client_id;
    int registered;
    int active;
};


// message settings:

#define RAW_MESSAGE_SIZE 64

// message types:

#define MSG_LOGIN       1   // format: "%d %s" -> msg_type, message
#define MSG_TASK        2   // format: "%d %d %d %d %d" -> msg_type, task_id, task_type, operand1, operand2, result
#define MSG_PING        3   // format: "%d %d" -> msg_type

struct login_message {

    char string[CLIENT_MAX_NAME_LENGTH];
};

struct task_message {

    int task_id;
    int task_type;
    int operand1;
    int operand2;
    int result;
};

struct ping_message {

    int ping_id;
};

// lib API:

int send_message(int socket_fd, const char message[RAW_MESSAGE_SIZE]);
int receive_message(int socket_fd, char message[RAW_MESSAGE_SIZE]);
int check_message_type(const char message[RAW_MESSAGE_SIZE]);

int send_login_message(int socket_fd, const char *string);
int send_task_message(int socket_fd, int task_id, int task_type, int operand1, int operand2, int result);
int send_ping_message(int socket_fd, int ping_id);

struct login_message get_login_message(const char raw_message[RAW_MESSAGE_SIZE]);
struct task_message get_task_message(const char raw_message[RAW_MESSAGE_SIZE]);
struct ping_message get_ping_message(const char raw_message[RAW_MESSAGE_SIZE]);

void error_check_neg1(int return_value, const char *message);

#endif
