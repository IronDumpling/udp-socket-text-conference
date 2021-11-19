// Basic
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
// TCP
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
// Multi-threading
#include <signal.h>
#include <pthread.h>
// Others
#include "message.h"

// Function Prototypes
void logIn(char**, int*, pthread_t*);
void logOut(int*, pthread_t*);
void joinSession (char**, int*);
void leaveSession (int);
void createSession (int);
void list (int);
void quit(int);
void sendText (char**, int);

// Global Constants
#define INVALID_SOCKET -1
const char *LOGIN_CMD = "/login";
const char *LOGOUT_CMD = "/logout";
const char *JOIN_SESSION_CMD = "/joinsession";
const char *LEAVE_SESSION_CMD = "/leavesession";
const char *CREATE_SESSION_CMD = "/createsession";
const char *LIST_CMD = "/list";
const char *QUIT_CMD = "/quit";

/**
 * Function 0. Client Main Function
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv)
{
    // Client program starts
    // Initialize
    int socketFD = INVALID_SOCKET;
    pthread_t recvThread;

    // Waiting for new commands
    while(true){
        // Get input
        // Corner Case 1: No Inputs
        if(!argc){
            continue;
        }

        // Command 1: log in
        if(!strcmp(argv[0], LOGIN_CMD)){
            logIn(argv, &socketFD, &recvThread);
        }
        // Command 2: log out
        else if(!strcmp(argv[0], LOGOUT_CMD)){
            logOut(&socketFD, &recvThread);
        }
        // Command 3: join session
        else if(!strcmp(argv[0], JOIN_SESSION_CMD)){
            joinSession(argv, &socketFD);
        }
        // Command 4: leave session
        else if(!strcmp(argv[0], LEAVE_SESSION_CMD)){
            leaveSession(socketFD);
        }
        // Command 5: create session
        else if(!strcmp(argv[0], CREATE_SESSION_CMD)){
            createSession(socketFD);
        }
        // Command 6: List Users
        else if(!strcmp(argv[0], LIST_CMD)){
            list(socketFD);
        }
        // Command 7: Quit
        else if(!strcmp(argv[0], QUIT_CMD)){
            quit(socketFD);
            break;
        }
        // Command 8: Send Text
        else{
            sendText(argv, socketFD);
        }
    }

    printf("Quit Text Conference Successfully.\n");

    return 0;
}

/**
 * Function 1. Log In
 * @param input
 * @param socketFD
 * @param recvThread
 */
void logIn(char **input, int *socketFD, pthread_t *recvThread) {

}

/**
 * Function 2. Log Out
 * @param socketFD
 * @param recvThread
 */
void logOut(int *socketFD, pthread_t *recvThread) {

}

/**
 * Function 3. Join Session
 * @param input
 * @param socketFD
 */
void joinSession (char **input, int *socketFD) {

}

/**
 * Function 4. Leave Session
 * @param socketFD
 */
void leaveSession (int socketFD) {

}

/**
 * Function 5. Create New Session
 * @param socketFD
 */
void createSession (int socketFD) {

}

/**
 * Function 6. List
 * @param socketFD
 */
void list (int socketFD) {

}

/**
 * Function 7. Quit
 * @param socketFD
 */
void quit(int socketFD) {

}

/**
 * Function 8.Send Text
 * @param input
 * @param socketFD
 */
void sendText (char **input, int socketFD) {

}