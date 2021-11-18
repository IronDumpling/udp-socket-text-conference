#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MAX_NAME 1000
#define MAX_DATA 1200

struct message{
    unsigned int type; // type of the message
    unsigned int size; // length of the data
    unsigned char source[MAX_NAME]; // ID of the client sending the message
    unsigned char data[MAX_DATA];
};

// Serialization & deserialization approaches

#endif
