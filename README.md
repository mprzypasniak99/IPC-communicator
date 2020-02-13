# IPC-COMUNICATOR
This project is a program for Linux, built in server-client architecture. It was developed as a project for concurrent programming. Server and clients communicate via message queues. While all requests are sent to server via one, "public" queue, it responds to each client via its private queue. GUI is not implemented, client uses console interface.

# COMPILATION:
Programs (server and client) are compiled by launching "compile.sh" script

# LAUNCHING:
## Server is compiled to file:
`server`

## Client is compiled to file:
`client`

# FILES:
- inf141302_s.c - contains code that handles every request sent to server, as well as its setup functions: creating account and group structures from file `config.txt`
- inf141302_c.c - contains all functionalities of a client : logging in and sending all requests to server
- inf141302_struct.h - contains structures that are used for sending and receiving messages by both programs
- config.txt - contains data that is used to create accounts and groups by server
