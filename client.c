// Basic
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
// TCP
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
// Multi-threading
//#include <signal.h>
#include <pthread.h>
// Others
#include "message.h"

// Function Prototypes
void logIn(char*, int*, pthread_t*);
void logOut(int*, pthread_t*);
void joinSession (char*, int*);
void leaveSession (int);
void createSession (char*, int);
void list (int);
void sendText (int);
void privateText (char *, int);
void registerUser(char*, int*);

// Global Constants
#define INVALID_SOCKET (-1)
bool inSession = false;
char recvBuff[BUFF_SIZE];

const char *LOGIN_CMD = "/login";
const char *LOGOUT_CMD = "/logout";
const char *JOIN_SESSION_CMD = "/joinsession";
const char *LEAVE_SESSION_CMD = "/leavesession";
const char *CREATE_SESSION_CMD = "/createsession";
const char *REGISTER_CMD = "/register";
const char *PRIVATE_CMD = "/private";
const char *LIST_CMD = "/list";
const char *QUIT_CMD = "/quit";

/**
 * Function 0. Client Main Function
 * @param argc
 * @param argv
 * @return
 */
int main()
{
    // Client program starts
    // Initialize
    char *input;
    unsigned int tokLength;
    int socketFD = INVALID_SOCKET;
    pthread_t recvThread;

    // Waiting for new commands
    while(true){
        // Get input to recvBuff from stdin
        fgets(recvBuff, BUFF_SIZE - 1, stdin);

        // Restore enough buf length for command
        // Change the \n bit into 0
        recvBuff[strcspn(recvBuff, "\n")] = 0;

        // Copy recvBuff to input
        input = recvBuff;

        // Trim
        while(*input == ' ') input++;

        // Corner Case 1: No Input
        if(*input == 0){
            continue;
        }

        // split recvBuff into sub-strings
        // input is the first sub-string
        input = strtok(recvBuff, " ");
        tokLength = strlen(input);

        // Command 1: log in
        if(!strcmp(input, LOGIN_CMD)){
            logIn(input, &socketFD, &recvThread);
        }
        // Command 2: log out
        else if(!strcmp(input, LOGOUT_CMD)){
            logOut(&socketFD, &recvThread);
        }
        // Command 3: join session
        else if(!strcmp(input, JOIN_SESSION_CMD)){
            joinSession(input, &socketFD);
        }
        // Command 4: leave session
        else if(!strcmp(input, LEAVE_SESSION_CMD)){
            leaveSession(socketFD);
        }
        // Command 5: create session
        else if(!strcmp(input, CREATE_SESSION_CMD)){
            createSession(input, socketFD);
        }
        // Command 6: Register New User
        else if(!strcmp(input, REGISTER_CMD)){
            registerUser(input, &socketFD);
        }
        // Command 7: List Users
        else if(!strcmp(input, LIST_CMD)){
            list(socketFD);
        }
        // Command 8: Quit
        else if(!strcmp(input, QUIT_CMD)){
            logOut(&socketFD, &recvThread);
            break;
        }
        // Command 9. Send Private Message
        else if(!strcmp(input, PRIVATE_CMD)){
            privateText(input, socketFD);
        }
        // Command 10: Send Text
        else{
            // restore recvBuff
            recvBuff[tokLength] = ' ';
            sendText(socketFD);
        }
    }

    printf("Quit Text Conference Successfully.\n");
    return 0;
}

/**
 * Helper 1.
 * Get Socket Address
 * @param socketAddr
 * @return
 */
void *getInAddr(struct sockaddr *socketAddr) {
    if(socketAddr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)socketAddr)->sin_addr);
    } else {
        return &(((struct sockaddr_in6*)socketAddr)->sin6_addr);
    }
}

/**
 * Helper 2.
 * Receive Message
 * @param socketVoidFD
 * @return
 */
void *receive(void *socketVoidFD) {
    int *socketFD = (int*)socketVoidFD;

    long length;
    struct message Message;

    while(true){
        // Receive Message
        length = recv(*socketFD, recvBuff, BUFF_SIZE - 1, 0);
        if(length == -1) {
            fprintf(stderr, "receive message failed.\n");
            return NULL;
        }

        // Corner Case
        if(!length) continue;

        recvBuff[length] = 0;
        stringToPacket(recvBuff, &Message);

        // Response to Message
        if (Message.type == JN_ACK) {
            fprintf(stdout, "Successfully joined session %s.\n", Message.data);
            inSession = true;
        } else if (Message.type == JN_NAK) {
            fprintf(stdout, "Join failure. Detail: %s\n", Message.data);
            inSession = false;
        } else if (Message.type == NS_ACK) {
            fprintf(stdout, "Successfully created and joined session %s.\n", Message.data);
            inSession = true;
        } else if (Message.type == QU_ACK) {
            fprintf(stdout, "User id\t\tSession ids\n%s", Message.data);
        } else if (Message.type == MESSAGE){
            fprintf(stdout, "%s: %s\n", Message.source, Message.data);
        } else {
            fprintf(stdout, "Unexpected packet received: type %d, data %s\n",
                    Message.type, Message.data);
        }
        // solve the ghost-newline
        fflush(stdout);
    }
}

/**
 * Function 1. Log In
 * @param input
 * @param socketFD
 * @param recvThread
 */
void logIn(char *input, int *socketFD, pthread_t *recvThread) {
    // Login Information
    char *clientID, *password, *serverIP, *serverPort;

    // Get Login Information
    // Get the rest sub-string
    input = strtok(NULL, " ");
    clientID = input;

    input = strtok(NULL, " ");
    password = input;

    input = strtok(NULL, " ");
    serverIP = input;

    input = strtok(NULL, " \n");
    serverPort = input;

    // Corner Case 1: Too Few Arguments
    if(!clientID || !password || !serverIP || !serverPort){
        printf("Too few arguments: /login <client_id> <password> <server_ip> <server_port>\n");
        return;
    }
    // Corner Case 2: Re-Login
    else if (*socketFD == INVALID_SOCKET) {
        // Step 1. Initialise TCP
        struct addrinfo hints, *servInfo;
        char s[INET6_ADDRSTRLEN];

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        // Step 2. Set up Address Info
        if(getaddrinfo(serverIP, serverPort, &hints, &servInfo) != 0){
            fprintf(stderr, "get address failed.\n");
            return;
        }

        // Step 3. Set up Connection (Until Succeed)
        struct addrinfo *p;

        for(p = servInfo; p != NULL; p = p->ai_next){
            // Get Socket
            *socketFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if(*socketFD == -1) {
                perror("Set client socket failed");
                continue;
            }

            // Set Connection
            if(connect(*socketFD, p->ai_addr, p->ai_addrlen) == -1) {
                close(*socketFD);
                perror("Connect server failed");
                continue;
            }
            break;
        }

        // Get Socket Info failed
        if(!p) {
            perror("Connect addrInfo Failed");
            close(*socketFD);
            *socketFD = INVALID_SOCKET;
            return;
        }

        // Get Socket Info Succeed
        inet_ntop(p->ai_family, getInAddr((struct sockaddr*)p->ai_addr), s, sizeof(s));
        printf("Connecting to %s\n", s);
        freeaddrinfo(servInfo);
    }

    // Normal Case
    // Step 4. Fill Message Struct
    struct message Message;
    Message.type = LOGIN;
    Message.size = strlen(password);
    strncpy(Message.source, clientID, MAX_NAME);
    strncpy(Message.data, password, MAX_DATA);

    // Serialization
    packetToString(&Message, recvBuff);

    // Step 5. Send Message
    if(send(*socketFD, recvBuff, BUFF_SIZE - 1, 0) ==  -1) {
        fprintf(stderr, "send message failed.\n");
        close(*socketFD);
        *socketFD = INVALID_SOCKET;
        return;
    }

    // Receive Message
    unsigned int length = recv(*socketFD, recvBuff, BUFF_SIZE - 1, 0);
    if(length == -1) {
        fprintf(stderr, "receive message failed.\n");
        close(*socketFD);
        *socketFD = INVALID_SOCKET;
        return;
    }

    // Deserialization
    stringToPacket(recvBuff, &Message);

    // Step 6. Response to Message
    if(Message.type == LO_ACK && pthread_create(recvThread, NULL, receive, socketFD) == 0) {
        printf("Login succeed.\n");
    } else if(Message.type == LO_NAK) {
        fprintf(stdout, "login failed. Detail: %s\n", Message.data);
        close(*socketFD);
        *socketFD = INVALID_SOCKET;
        return;
    } else {
        fprintf(stdout, "unexpected message received. "
                        "Detail: type %d, data %s\n", Message.type, Message.data);
        close(*socketFD);
        *socketFD = INVALID_SOCKET;
        return;
    }
}

/**
 * Function 2. Log Out
 * @param socketFD
 * @param recvThread
 */
void logOut(int *socketFD, pthread_t *recvThread) {
    if(*socketFD == INVALID_SOCKET){
        perror("Haven't login yet");
        return;
    }

    struct message Message;
    Message.type = EXIT;
    Message.size = 0;

    packetToString(&Message, recvBuff);

    if(send(*socketFD, recvBuff, BUFF_SIZE - 1, 0) == -1) {
        fprintf(stderr, "send message failed.\n");
        return;
    }

    if(pthread_cancel(*recvThread) != 0) {
        fprintf(stderr, "logout failed.\n");
    } else {
        fprintf(stdout, "logout succeed.\n");
    }

    inSession = false;
    close(*socketFD);
    *socketFD = INVALID_SOCKET;
}

/**
 * Function 3. Join Session
 * @param input
 * @param socketFD
 */
void joinSession (char *input, int *socketFD) {
    // Corner Case 1.
    if(*socketFD == INVALID_SOCKET) {
        perror("Haven't login yet");
        return;
    }
    // Corner Case 2.
    else if (inSession){
        perror("Already joined a session");
        return;
    } else{
        // Get Input
        input = strtok(NULL, " ");
        char *sessionID = input;

        // Corner Case 3.
        if(!sessionID){
            printf("Too few arguments: /joinsession <session_id>\n");
        }
        // Common Case
        else{
            // Fill Message struct
            struct message Message;
            Message.type = JOIN;
            Message.size = strlen(sessionID);
            strcpy(Message.data, sessionID);

            // Serialization
            packetToString(&Message, recvBuff);

            // Send to Server
            if(send(*socketFD, recvBuff, BUFF_SIZE - 1, 0) == -1){
                perror("Send Message Failed");
                return;
            }

            // Update bool
            inSession = true;
        }
    }
}

/**
 * Function 4. Leave Session
 * @param socketFD
 */
void leaveSession (int socketFD) {
    // Corner Case 1.
    if(socketFD == INVALID_SOCKET) {
        fprintf(stdout, "haven't login yet.\n");
        return;
    }
    // Corner Case 2.
    else if (!inSession){
        fprintf(stdout, "haven't joined a session yet.\n");
        return;
    } else{
        // Fill Message struct
        struct message Message;
        Message.type = LEAVE_SESS;
        Message.size = 0;

        // Serialization
        packetToString(&Message, recvBuff);

        // Send to Server
        if(send(socketFD, recvBuff, BUFF_SIZE - 1, 0) == -1){
            perror("Send Message Failed");
            return;
        }

        // Update bool
        inSession = false;
    }
}

/**
 * Function 5. Create New Session
 * @param socketFD
 */
void createSession (char *input, int socketFD) {
    // Corner Case 1.
    if(socketFD == INVALID_SOCKET) {
        fprintf(stdout, "haven't login yet.\n");
        return;
    }
    // Corner Case 2.
    else if (inSession){
        fprintf(stdout, "already joined a session.\n");
        return;
    }
    // Corner Case 3.
    input = strtok(NULL, " ");
    char *sessionID = input;

    if (!sessionID){
        fprintf(stdout, "usage: /createsession <session_id>\n");
        return;
    }
    // Normal Case
    else{
        // Fill Message struct
        struct message Message;
        Message.type = NEW_SESS;
        // Message.size = 0;
        Message.size = strlen(sessionID);
        strcpy(Message.data, sessionID);

        // Serialization
        packetToString(&Message, recvBuff);

        // Send to Server
        if(send(socketFD, recvBuff, BUFF_SIZE - 1, 0) == -1){
            fprintf(stderr, "send message failed.\n");
            return;
        }
    }
}

/**
 * Function 6. List
 * @param socketFD
 */
void list (int socketFD) {
    // Corner Case 1.
    if(socketFD == INVALID_SOCKET) {
        fprintf(stdout, "haven't login yet.\n");
        return;
    }
    // Common Case
    else{
        // Fill Message struct
        struct message Message;
        Message.type = QUERY;
        Message.size = 0;

        // Serialization
        packetToString(&Message, recvBuff);

        // Send to Server
        if(send(socketFD, recvBuff, BUFF_SIZE - 1, 0) == -1){
            fprintf(stderr, "send message failed.\n");
            return;
        }
    }
}

/**
 * Function 7.Send Text
 * @param input
 * @param socketFD
 */
void sendText (int socketFD) {
    // Corner Case 1.
    if(socketFD == INVALID_SOCKET) {
        fprintf(stdout, "haven't login yet.\n");
        return;
    }
    // Corner Case 2.
    else if(!inSession) {
        fprintf(stdout, "haven't joined a session yet.\n");
        return;
    }

    // Common Case
    else{
        // Fill Message struct
        struct message Message;
        Message.type = MESSAGE;
        Message.size = strlen(recvBuff);
        strncpy(Message.data, recvBuff, MAX_DATA);

        // Serialization
        packetToString(&Message, recvBuff);

        // Send to Server
        if(send(socketFD, recvBuff, BUFF_SIZE - 1, 0) == -1){
            fprintf(stderr, "send message failed.\n");
            return;
        }
    }
}

/**
 * Function 8. Send Private Text
 * @param socketFD
 */
void privateText (char *input, int socketFD) {
    // Corner Case 1.
    if(socketFD == INVALID_SOCKET) {
        fprintf(stdout, "haven't login yet.\n");
        return;
    }

    // Common Case
    else{
        // Login Information
        char *clientID, *text;

        // Get Login Information
        // Get the rest sub-string
        input = strtok(NULL, " ");
        clientID = input;

        input = strtok(NULL, " \n");
        text = input;

        // Corner Case 1: Too Few Arguments
        if(!clientID || !text){
            printf("Too few arguments: /private <client_id> <text>\n");
            return;
        }

        // Fill Message struct
        struct message Message;
        Message.type = PRIV_MESSAGE;
        Message.size = strlen(recvBuff);
        strncpy(Message.data, clientID, MAX_DATA);
        strncpy(Message.data, text, MAX_DATA);

        // Serialization
        packetToString(&Message, recvBuff);

        // Send to Server
        if(send(socketFD, recvBuff, BUFF_SIZE - 1, 0) == -1){
            fprintf(stderr, "send message failed.\n");
            return;
        }
    }
}

/**
 * Function 9. Register New User
 * @param input
 * @param socketFD
 */
void registerUser(char *input, int *socketFD) {
    // Login Information
    char *clientID, *password, *serverIP, *serverPort;

    // Get Login Information
    // Get the rest sub-string
    input = strtok(NULL, " ");
    clientID = input;

    input = strtok(NULL, " ");
    password = input;

    input = strtok(NULL, " ");
    serverIP = input;

    input = strtok(NULL, " \n");
    serverPort = input;

    // Corner Case 1: Too Few Arguments
    if(!clientID || !password  || !serverIP || !serverPort){
        printf("Too few arguments: /register <client_id> <password> <server_ip> <server_port>\n");
        return;
    }
    // Corner Case 2: Haven't Log-in
    else if (*socketFD == INVALID_SOCKET) {
        // Step 1. Initialise TCP
        struct addrinfo hints, *servInfo;
        char s[INET6_ADDRSTRLEN];

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        // Step 2. Set up Address Info
        if(getaddrinfo(serverIP, serverPort, &hints, &servInfo) != 0){
            fprintf(stderr, "get address failed.\n");
            return;
        }

        // Step 3. Set up Connection (Until Succeed)
        struct addrinfo *p;

        for(p = servInfo; p != NULL; p = p->ai_next){
            // Get Socket
            *socketFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if(*socketFD == -1) {
                perror("Set client socket failed");
                continue;
            }

            // Set Connection
            if(connect(*socketFD, p->ai_addr, p->ai_addrlen) == -1) {
                close(*socketFD);
                perror("Connect server failed");
                continue;
            }
            break;
        }

        // Get Socket Info failed
        if(!p) {
            perror("Connect addrInfo Failed");
            close(*socketFD);
            *socketFD = INVALID_SOCKET;
            return;
        }

        // Get Socket Info Succeed
        inet_ntop(p->ai_family, getInAddr((struct sockaddr*)p->ai_addr), s, sizeof(s));
        printf("Connecting to %s\n", s);
        freeaddrinfo(servInfo);
    }

    // Normal Case
    // Step 4. Fill Message Struct
    struct message Message;
    Message.type = REGISTER;
    Message.size = strlen(password);
    strncpy(Message.source, clientID, MAX_NAME);
    strncpy(Message.data, password, MAX_DATA);

    // Serialization
    packetToString(&Message, recvBuff);

    // Step 5. Send Message
    if(send(*socketFD, recvBuff, BUFF_SIZE - 1, 0) ==  -1) {
        fprintf(stderr, "send message failed.\n");
        close(*socketFD);
        *socketFD = INVALID_SOCKET;
        return;
    }

    // Receive Message
    unsigned int length = recv(*socketFD, recvBuff, BUFF_SIZE - 1, 0);
    if(length == -1) {
        fprintf(stderr, "receive message failed.\n");
        close(*socketFD);
        *socketFD = INVALID_SOCKET;
        return;
    }

    // Deserialization
    stringToPacket(recvBuff, &Message);

    // Step 6. Response to Message
    if(Message.type == LO_ACK) {
        printf("Register succeed.\n");
    } else if(Message.type == LO_NAK) {
        fprintf(stdout, "register failed. Detail: %s\n", Message.data);
        close(*socketFD);
        *socketFD = INVALID_SOCKET;
        return;
    } else {
        fprintf(stdout, "unexpected message received. "
                        "Detail: type %d, data %s\n", Message.type, Message.data);
        close(*socketFD);
        *socketFD = INVALID_SOCKET;
        return;
    }
}

