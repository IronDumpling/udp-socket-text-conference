/* external libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "message.h"
#include "database.h"


/* global variables */
pthread_mutex_t lock_session_list = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_online_user = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_session_count = PTHREAD_MUTEX_INITIALIZER;
int session_count = 1;
User *user_list = NULL;
User *online_user_list = NULL;
Session *session_list = NULL;


/* called when new connections are established. */
void *stub_client(void *arg) {
    /* variables initialization */
    User *new_client = (User*)arg;
    char buffer[BUFF_SIZE];
    char source[MAX_NAME];
    int msg_len;
    struct message recv_packet;
    struct message send_packet;
    bool online = false;
    bool to_exit = false;

    printf("A new thread has been created for the new client.\n");
    
    bool to_send = false;
    
    /* start to hear from the client */
    while(true) {
        memset(buffer, 0, BUFF_SIZE);
        memset(&recv_packet, 0, sizeof(struct message));
        memset(&send_packet, 0, sizeof(struct message));		

        msg_len = recv(new_client->sockfd, buffer, BUFF_SIZE - 1, 0);
        if (msg_len < 0) {
            perror("Failed at recv...\n");
            exit(1);
        } else if (msg_len == 0) {
            to_exit = true;
        } else {
            stringToPacket(buffer, &recv_packet);
            printf("--------------------\n");
            printf("Request received.\n");
        }
        
        buffer[msg_len] = '\0';
        to_send = false;

        if (recv_packet.type == EXIT) to_exit = true;

        /* when not online, only "login" & "exit" are available. */
        if(!online) {
            if(recv_packet.type != LOGIN) {
                send_packet.type = LO_NAK;
                to_send = true;
                strcpy((send_packet.data), "Please login before making other manipulations.");
            } else {
                strcpy(new_client->UID, (char *)(recv_packet.source));
                strcpy(new_client->password, (char *)(recv_packet.data));
                printf("User ID: %s\n", new_client->UID);
                printf("User password: %s\n", new_client->password);

                /* check if this client has logged in */
                bool valid_user = user_check(user_list, new_client, 0);
                bool multiple_login = user_check(online_user_list, new_client, 1);
                bool password_correctness = user_check(user_list, new_client, 1);

                if(valid_user && !multiple_login) {
                    strcpy(source, new_client->UID);
                    printf("User ID: %s, has successfully logged in.\n", new_client->UID);
                    
                    to_send = true;
                    online = true;
                    send_packet.type = LO_ACK;

                    /* append this client to online client list */
                    User *tmp = malloc(sizeof(User));
                    memcpy(tmp, new_client, sizeof(User));
                    
                    pthread_mutex_lock(&lock_online_user);
                    online_user_list = add_user(tmp, online_user_list);
                    pthread_mutex_unlock(&lock_online_user);
                } else {
                    to_send = true;
                    to_exit = true;
                    send_packet.type = LO_NAK;
                    /* fill the data field to specify the reason of failure */
                    if (multiple_login) {
                        strcpy((char *)(send_packet.data), "This account is already online.");
                        printf("User ID is invalid due to multiple login. Feedback has been sent to this anonymous client.\n");
                    } else if(password_correctness) {	
                        strcpy((char *)(send_packet.data), "The password is not consistent.");
                        printf("User ID is invalid due to incorrect password. Feedback has been sent to this anonymous client.\n");
                    } else {
                        strcpy((char *)(send_packet.data), "This UID has not been registered.");
                        printf("User ID is invalid due to unregistered UID. Feedback has been sent to this anonymous client.\n");
                    }
                }
            }
        } else if(recv_packet.type == JOIN) {
            int session_id = atoi((recv_packet.data));
            printf("User %s is requesting to join session NO.%d...\n", new_client->UID, session_id);

            to_send = true;
            /* nonexistent session */
            if(!session_check(session_list, session_id)) {
                send_packet.type = JN_NAK;
                strcpy(send_packet.data, "nonexistent session");
                printf("User %s Failed to join session NO.%d due to nonexistent session.\n", new_client->UID, session_id);
            } 
            /* multiple joining */
            else if(user_in_session(session_list, session_id, new_client)) {
                send_packet.type = JN_NAK;
                strcpy(send_packet.data, "multiple joining");
                printf("User %s Failed to join session NO.%d due to multiple joining.\n", new_client->UID, session_id);
            }
            /* successfully joining */
            else {
                send_packet.type = JN_ACK;
                strcpy(send_packet.data, recv_packet.data);
                
                /* update sessions the current client has joined */
                new_client->session_enrolment = add_session(new_client->session_enrolment, session_id);

                /* insert this user into the corresponding session */
                pthread_mutex_lock(&lock_session_list);
                session_list = join_session(session_list, session_id, new_client);
                pthread_mutex_unlock(&lock_session_list);
                
                printf("User %s Successfully joined session NO.%d.\n", new_client->UID, session_id);

                /* update online user status */
                pthread_mutex_lock(&lock_online_user);
                for(User *p = online_user_list; p; p = p->next) {
                    if(strcmp(source, p->UID) == 0) {
                        p->session_enrolment = add_session(p->session_enrolment, session_id);
                        p->in_session = true;
                        break;
                    }
                }
                pthread_mutex_unlock(&lock_online_user);
            }
        } else if(recv_packet.type == LEAVE_SESS) {
            printf("User %s is requesting to leave all joined sessions.\n", new_client->UID);

            /* delete current joined session from the user */
            if (new_client->session_enrolment != NULL) {
                pthread_mutex_lock(&lock_session_list);
                session_list = leave_session(session_list, new_client->session_enrolment->SID, new_client);
                pthread_mutex_unlock(&lock_session_list);
                printf("User %s has left session %d.\n", new_client->UID, new_client->session_enrolment->SID);
                new_client->session_enrolment = NULL;
                new_client->in_session = false;
            }

            /* update global user state */
            pthread_mutex_lock(&lock_online_user);
            for(User *p = online_user_list; p; p = p->next) {
                if(strcmp(p->UID, source) == 0) {
                    p->session_enrolment = NULL;
                    p->in_session = false;
                    break;
                }
            }
            pthread_mutex_unlock(&lock_online_user);
        } else if(recv_packet.type == NEW_SESS) {
            printf("User %s is requesting to create a new session.\n", new_client->UID);
            
            send_packet.type = NS_ACK;
            to_send = true;
            sprintf((char *)(send_packet.data), "%d", session_count);

            /* Update global session_list */
            pthread_mutex_lock(&lock_session_list);
            session_list = add_session(session_list, session_count);
            pthread_mutex_unlock(&lock_session_list);

            /* User join just created session */
            pthread_mutex_lock(&lock_session_list);
            session_list = join_session(session_list, session_count, new_client);
            new_client->session_enrolment = add_session(new_client->session_enrolment, session_count);
            pthread_mutex_unlock(&lock_session_list);

            /* update online user status */
            pthread_mutex_lock(&lock_online_user);
            for(User *p = online_user_list; p; p = p->next) {
                if(strcmp(p->UID, source) == 0) {
                    p->in_session = true;
                    p->session_enrolment = add_session(p->session_enrolment, session_count);
                    break;
                }
            }
            pthread_mutex_unlock(&lock_online_user);

            /* update session count */
            pthread_mutex_lock(&lock_session_count);
            session_count++;
            pthread_mutex_unlock(&lock_session_count);

            printf("User %s has successfully created session NO.%d.\n", new_client->UID, session_count - 1);
        } else if(recv_packet.type == MESSAGE) {
            printf("User %s is requesting to send message.\n", new_client->UID);
            bool cc = false;
            pthread_mutex_lock(&lock_online_user);
            for(User *p = online_user_list; p; p = p->next) {
                if(strcmp(p->UID, source) == 0) {
                    if (!p->in_session) {
                        cc = true;
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&lock_online_user);
            
            if (cc) {
                printf("This user is currently not in a session.\n");
                continue;
            }
            
            to_send = false;
            /* pack up the packet to send */
            send_packet.type = MESSAGE;
            strcpy(send_packet.source, new_client->UID);
            strcpy(send_packet.data, recv_packet.data);
            send_packet.size = strlen(send_packet.data);

            memset(buffer, 0, BUFF_SIZE);
            packetToString(&send_packet, buffer);

            /* broadcast */
            Session *sess_to_send = session_check(session_list, new_client->session_enrolment->SID);
            if(sess_to_send == NULL) continue;
            for(User *curr_user = sess_to_send->enrol_list; curr_user; curr_user = curr_user->next) {
                int sd = send(curr_user->sockfd, buffer, BUFF_SIZE-1, 0);
                if(sd < 0) {
                    perror("Failed when sending...\n");
                    exit(1);
                }
            }
        } else if (recv_packet.type == QUERY) {
            printf("User %s is requesting to query a list.\n", new_client->UID);

            int cursor = 0;
            send_packet.type = QU_ACK;
            to_send = true;

            for(User *p = online_user_list; p; p = p->next) {
                cursor += sprintf(send_packet.data + cursor, "%s", p->UID);
                for(Session *ss = p->session_enrolment; ss; ss = ss->next) {
                    cursor += sprintf(send_packet.data + cursor, "\t%d", ss->SID);
                }
                send_packet.data[cursor++] = '\n';
            }

            printf("Query result has been sent to the client.\n");
        }

        /* send packet back if applied */
        if(to_send) {
            memcpy(send_packet.source, new_client->UID, 32);
            send_packet.size = strlen(send_packet.data);
            memset(buffer, 0, BUFF_SIZE);
            packetToString(&send_packet, buffer);
            msg_len = send(new_client->sockfd, buffer, BUFF_SIZE-1, 0);
            if(msg_len < 0) {
                perror("Failed when sending...\n");
            }
        }
        
        if(to_exit) break;
    }

    close(new_client->sockfd);

    /* client quits, so remove it from any data structure containing it */
    if(!online) return NULL;
    
    printf("User %s has exited.\n", new_client->UID);

    for(Session *curr = new_client->session_enrolment; curr; curr = curr->next) {
        pthread_mutex_lock(&lock_session_list);
        session_list = leave_session(session_list, curr->SID, new_client);
        pthread_mutex_unlock(&lock_session_list);
    }

    for(User *p = online_user_list; p; p = p->next) {
        if(strcmp(source, p->UID) == 0) {
            free_session_list(p->session_enrolment);
            break;
        }
    }

    online_user_list = remove_user(online_user_list, new_client);

    free_session_list(new_client->session_enrolment);

    free(new_client);
}


int main() {
    /* fetch user input */
    printf("Server activation format: server <port number>\n");
    char input[32];
    memset(input, 0, 32 * sizeof(char));
    fgets(input, 32, stdin);
    char port[6];
    memcpy(port, input + 7, strlen(input) - 8);
    printf("Server starts at port %s.\n", port);

    /* read the user id & password file */
    FILE* f_ul = fopen("user_data", "r");
    while (true) {
        User* unit = malloc(sizeof(User));
        if (fscanf(f_ul, "%s %s\n", unit->UID, unit->password) != EOF) {
            user_list = add_user(unit, user_list);
        } else {
            free(unit);
            break;
        }
    }
    fclose(f_ul);
    printf("Server has loaded available user accounts.\n");

    /* setup the initial server socket */
    struct addrinfo *svinfo;
    struct sockaddr_storage client_addr; // connector's address information

    /* configure address information */
    struct addrinfo addr;
    memset(&addr, 0, sizeof(addr));
    addr.ai_family = AF_INET;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, port, &addr, &svinfo);

    /* loop and bind the first available one */
    int sockfd;
    int optval = 1;
    struct addrinfo *curr = svinfo;
    for(; curr; curr = curr->ai_next) {
        sockfd = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (sockfd < 0) {
            perror("Failed to configure socket for the current resource, try the next...");
        } else {
            int sskt = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
            if (sskt < 0) {
                perror("Failed at setsockopt...");
                exit(1);
            } else {
                int bd = bind(sockfd, curr->ai_addr, curr->ai_addrlen);
                if (bd < 0) {
                    close(sockfd);
                    perror("Failed when binding socket...");
                } else {
                    break;
                }
            }
        }
    }
    freeaddrinfo(svinfo);
    if (curr == NULL)  {
        fprintf(stderr, "Failed when binding...\n");
        exit(1);
    }

    /* Listen on incoming port, 12 connections available in the queue. */
    int ls = listen(sockfd, 12);
    if (ls < 0) {
        perror("Failed to listen...");
        exit(1);
    }
    
    printf("Server is ready for connections.\n");

    /* listen and accept connections */
    while(true) {
        User *new_client = malloc(sizeof(User));
        socklen_t sin_size = sizeof(client_addr);
        new_client->sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (new_client->sockfd < 0) {
            perror("Failed at accept()...");
            break;
        }

        /* create a new thread */
        pthread_create(&(new_client->thu), NULL, stub_client, (void *)new_client);
    }

    /* release allocated memory */
    free_user_list(user_list);
    free_user_list(online_user_list);
    free_session_list(session_list);
    close(sockfd);
    printf("Server quits...\n");
    return 0;
}
