//
// Created by 张楚岳 on 2021-11-19.
//

#ifndef SESSION_H
#define SESSION_H

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "user.h"

struct session {
    int sessionId;
    struct session *next; // Linked list of sessions
    struct user *user;	  // Users that joined this session
};

#endif
