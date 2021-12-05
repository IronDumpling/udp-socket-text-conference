# ECE361TextConferenceLab
## ECE 361 Lab 4 &amp; 5
---
## command list
### User Login 
  /login <client_id> <password> <server_ip> <server_port>
### User Register
  /register <client_id> <password> <server_ip> <server_port>
### User Logout
  /logout
### Join Session
  /joinsession <session_id>
### Leave Current Session
  /leavesession
### Create New Session
  /createsession <session_id>
### Send Private Message
  /private <client_id> <text_message>
### List Online User Status
  /list
### User Logout and Terminate Program
  /quit

  # ECE361TextConferenceLab by Da Ma & Chuyue Zhang

The 2 new features are: private messaging & user registration.

------------------------------------------------

1. Private Messaging
This function works in 4 general conditions:
First, if the format of the command line input is incorrect, there will be a message prompting the client with the legal format.
Second, if the desired receiver account is non-existent (never been registered yet), the message will be blocked by the server. The server will also inform the sender by sending back a message.
Third, if the desired receiver is currently offline, the message will also be blocked by the server. The server will further inform the sender by sending back a message.
Fourth, if the receiver is online, the private message will be delivered regardless of sessions.

The sender fills in the source field of the packet with the receiver's UID, the data field with the content of message, and mark the type of the message as PRIV_MESSAGE. This packet will be sent to the server directly.
The server extracts each fields, and check if the receiver UID is legal. If legal, server will pack up a new packet with the sender's UID and content to the receiver. Corresponding feedback messages will also be sent back to the sender.

------------------------------------------------

2. User Registration
As described in the handout, this function can provide a way to register an account when the client does not have one (in other words, the client uses this function without logging in).

The client will pack the UID and password in the source and data fields respectively.
The server will extracted the UID and password from the packet, and look up a internal linked list to check if the UID has been occupied.
If it is available, the server will do some system calls to access the file which contains user account information and append the new account into a new line.
After that, server will also update the linked list and other possibly related data structures.

The client can then naturally login with or without restarting the server or client.

------------------------------------------------
