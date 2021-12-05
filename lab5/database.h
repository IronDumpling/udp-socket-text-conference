#ifndef DATABASE_H_
#define DATABASE_H_

/* source included */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

/* struct type def */
typedef struct user_struct User;
typedef struct session_struct Session;

struct user_struct {
    /* user name & password */
    char UID[32];
    char password[32];
    
    /* session status */
    bool in_session;
    Session *session_enrolment;
    
    /* thread parameter */
    pthread_t thu;

    /* socket connection */
    int sockfd;

    /* linked list */
    User *next;
};

struct session_struct {
    /* session id */
    int SID;
    
    /* user list in this session*/
    User *enrol_list;

    /* linked list */
    Session *next;
};


/* get address information depending on IP version: IPv4 or IPv6 */
void *get_addr_info(struct sockaddr *addr) {
    if (addr->sa_family == AF_INET)
        return &(((struct sockaddr_in*)addr)->sin_addr);
    else return &(((struct sockaddr_in6*)addr)->sin6_addr);
}


/* add user to a user list */
User *add_user(User *new_client, User* user_list) {
    new_client->next = user_list;
    return new_client;
}


/* remove user from a user list */
User *remove_user(User *user_list, User const *curr_user) {
    if(user_list == NULL) return NULL;
    
    if(strcmp(user_list->UID, curr_user->UID) == 1) {
        User *curr = user_list;
        User *prev = user_list;
        while(curr) {
            if(strcmp(curr->UID, curr_user->UID) == 0) {
                prev->next = curr->next;
                free(curr);
                break;
            }
            prev = curr;
            curr = curr->next;
        }
        return user_list;
    } else {
        User *ret = user_list->next;
        free(user_list);
        return ret;
    }
}


/* release the user list */
int free_user_list(User *root) {
    User *current = root;
    User *next;
    while(current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    return 0;
}


/* check validity of UID & password or existence */
bool user_check(const User *user_list, const User *curr_user, int mode) {
    const User *curr = user_list;
    /* traverse the target list to see if the user is an element of it (existence) */
    while (curr) {
        /* check both UID & password */
        if((mode == 0 && strcmp(curr->UID, curr_user->UID) == 0 
                && strcmp(curr -> password, curr_user -> password) == 0) 
                || (mode == 1 && strcmp(curr->UID, curr_user->UID) == 0))
            return true;
        curr = curr->next;
    }
    /* if the program reaches here, non-existence occurs */
    return false;
}


/* check if a session exists */
Session *session_check(Session *session_list, int session_id) {
    Session *curr = session_list;
    while(curr) {
        if(curr->SID == session_id)
            return curr;
        curr = curr->next;
    }
    return NULL;
}


/* check if a user is in a session */
bool user_in_session(Session *session_list, int session_id, const User *user) {
    Session *session = session_check(session_list, session_id);
    if(session) return user_check(session->enrol_list, user, 1);
    else return false;
}


/* initialize a session */
Session *add_session(Session *session_list, int session_id) {
    Session *newSession = malloc(sizeof(Session));
    newSession->SID = session_id;
    newSession->next = session_list;
    return newSession;
}


/* Remove a session from list */
Session* remove_session(Session* session_list, int session_id) {
    if (session_list == NULL) return NULL;

    if(session_list->SID == session_id) {
        Session *curr = session_list->next;
        free(session_list);
        return curr;
    } else {
        Session *curr = session_list;
        Session *prev;
        while(curr != NULL) {
            if(curr->SID != session_id) {
                prev = curr;
                curr = curr->next;
            } else {
                prev->next = curr->next;
                free(curr);
                break;
            }
        }
        return session_list;
    }
}


/* insert a user into a session */
Session *join_session(Session *session_list, int session_id, const User *user) {
    Session *curr = session_check(session_list, session_id);
    if (curr == NULL) return NULL;

    /* allocate a copy of the user to avoid corruption */
    User *new_user = malloc(sizeof(User));
    memcpy(new_user, user, sizeof(User));
    
    curr->enrol_list = add_user(new_user, curr->enrol_list);

    return session_list;
}


/* leave session for a user */
Session *leave_session(Session *session_list, int session_id, const User *user) {
    Session *curr = session_check(session_list, session_id);
    if (curr == NULL) return NULL;

    if (user_check(curr->enrol_list, user, 1) == false) return NULL;
    
    curr->enrol_list = remove_user(curr->enrol_list, user);

    if(curr->enrol_list == NULL)
        session_list = remove_session(session_list, session_id);
    
    return session_list;
}


/* delete a session list */
void free_session_list(Session *session_list) {
    Session *curr = session_list;
    Session *next = curr;
    while(curr) {
        free_user_list(curr->enrol_list);
        next = curr->next;
        free(curr);
        curr = next;
    }
}


#endif