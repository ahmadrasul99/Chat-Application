Ahmad Rasul
Project 2
04/10/2021

/* DISCLAIMER */
All the functionalities required are implemented except for program initialization using configuration_file.

Server Program

The server program is implemented with select() functionality to support multiple clients. Up to 30 clients can be supported at one time. I created a clientSocket structure, which holds each client's socket file descriptor and the username associated with the socket. This allows me to either broadcast a message sent from a client to all clients or facilitate chat between two clients in private. If the client enters the logout command, the socket is closed and the client is informed.

Client Program

The client program is implemented using pthreads. Every time a client inputs the login command, the program creates a pthread and creates a new socket. This socket is then connected to the server. If the connection is successful, the pthread enters a while loop which only takes in either the chat or the logout command. If the client enters the the logout command, it signals the server and closes the socket and thread.

Signal Handling

Implemented in the client program. Upon encountering SIGINT, the program calls the signalHandler function, which closes any open sockets or threads.

While the ability to read from configuration_file is implemented in both programs, it is not used. Instead, both client and server are assigned port number 25100 and the IP address of linprog7.cs.fsu.edu server. The server program's master socket connects to an address using INADDR_ANY while the client program's socket connects to localhost.
