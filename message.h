#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define MAX_NAME 32
#define MAX_DATA 512
#define BUF_SIZE 1000

// Message Struct
struct message{
    unsigned int type; // type of the message
    unsigned int size; // length of the data
    unsigned char source[MAX_NAME]; // ID of the client sending the message
    unsigned char data[MAX_DATA];
};

// Message Type
enum messageType{
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    MESSAGE,
    QUERY,
    QU_ACK
};

/**
 * Function 1. Serialization
 * @param msg
 * @param dest
 */
void packetToString(const struct message *msg, char *dest) {
    // Initialize buffer
    memset(dest, '\0', sizeof(char) * BUF_SIZE);

    unsigned long index;

    // Add type
    sprintf(dest, "%d", msg->type);
    index = strlen(dest);
    dest[index++] = ':';

    // Add size
    sprintf(dest + index, "%d", msg->size);
    index = strlen(dest);
    dest[index++] = ':';

    // Add source
    sprintf(dest + index, "%s", msg->source);
    index = strlen(dest);
    dest[index++] = ':';

    // Add data
    memcpy(dest + index, msg->data, strlen((char *)(msg -> data)));
}

/**
 * Function 2. Deserialization
 * @param src
 * @param destMsg
 */
void stringToPacket(const char *src, struct message *destMsg){
    int start = 0, end = 0;
    char* buf;

    // Extract type number from first Position
    while(src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    destMsg->type = atoi(buf);
    end++;
    start = end;
    free(buf);

    // Extract size number from second Position
    while(src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    destMsg->size = atoi(buf);
    end++;
    start = end;
    free(buf);

    // Extract source from third Position
    while(src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    //destMsg->source[] = 0; Not finished
    end++;
    start = end;
    free(buf);

    // Lastly, Extract data from Position left
    memcpy(destMsg->data, &src[start], destMsg->size);
}

#endif
