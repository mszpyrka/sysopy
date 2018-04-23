#ifndef SERVER_INFO_H
#define SERVER_INFO_H


#define SI_CLIENTS_LIMIT    30

#define SI_IPC_KEY_NUMBER   15

#define SI_REQ_REGISTER     0
#define SI_REQ_MIRROR       1
#define SI_REQ_CALC         2
#define SI_REQ_TIME         3
#define SI_REQ_STOP         4   // Sent by clients before their termination
#define SI_REQ_END          5   // Sent to request server termination


#define SI_SERVTERM         5

#define SI_MESSAGE_MAX_SIZE 256


#endif
