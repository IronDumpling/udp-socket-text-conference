#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define MAX_NAME 32
#define MAX_DATA 512
#define BUFF_SIZE 1000

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
    memset(dest, '\0', sizeof(char) * BUFF_SIZE);

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
void stringToPacket(const char *src, struct message *dest_packet){
    memset(dest_packet -> data, 0, MAX_DATA);
    if(strlen(src) == 0) return;

    // Compile Regex to match ":"
    regex_t regex;
    if(regcomp(&regex, "[:]", REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
    }

    // Match regex to find ":"
    regmatch_t pmatch[1];
    long long cursor = 0;
    const int regBfSz = MAX_DATA;
    char buf[regBfSz];     // Temporary buffer for regex matching

    // Match type
    if(regexec(&regex, src + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memset(buf, 0, regBfSz * sizeof(char));
    memcpy(buf, src + cursor, pmatch[0].rm_so);
    dest_packet -> type = atoi(buf);
    cursor += (pmatch[0].rm_so + 1);

    // Match size
    if(regexec(&regex, src + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memset(buf, 0, regBfSz * sizeof(char));
    memcpy(buf, src + cursor, pmatch[0].rm_so);
    dest_packet -> size = atoi(buf);
    cursor += (pmatch[0].rm_so + 1);

    // Match source
    if(regexec(&regex, src + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "Error matching regex\n");
        exit(1);
    }
    memcpy(dest_packet -> source, src + cursor, pmatch[0].rm_so);
    dest_packet -> source[pmatch[0].rm_so] = 0;
    cursor += (pmatch[0].rm_so + 1);

    // Match data
    memcpy(dest_packet -> data, src + cursor, dest_packet -> size);
}

#endif
