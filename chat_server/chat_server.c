#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// global variables
char * command[100]; // stores current command

// function declarations
void parseSpace(char *, char **);

// structure to store username and associated socket descriptor
struct clientSocket
{
  int socket;
  char * username;
} clientSocket[30];

int main(int argc, char * argv[])
{
  // error checking: wrong usage
  if (argc != 2) {
    printf("Usage: client configuration_file\n");
    return 0;
  }

  // read configuration_file
  FILE * file;

  // error checking: wrong file passed in
  if (strcmp(argv[1], "configuration_file") != 0) {
    printf("Wrong file passed in!\n");
    return 0;
  }

  // open file
  file = fopen(argv[1], "r");

  // error checking: file not opened properly
  if (file == NULL) {
    printf("Error while opening file!\n");
    return 0;
  }

  // read from file

  // close file
  fclose(file);

  fd_set readfds; // set of socket descriptors
  int sd, max_sd; // socket descriptor variables
  int activity; // set activity variable

  // initialize all client sockets
  int i, maxClients = 30;
  for (i = 0; i < maxClients; i++) {
    clientSocket[i].socket = 0;
  }

  // buffer to read in from client
  char buffer[1024];

  // setup socket variables
  int masterSocket, newSocket;

  // create a master socket
  if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Master socket creation failed!\n");
    return  0;
  }
  else
    printf("Master socket created!\n");

  // initialize server structure
  struct sockaddr_in server;
  memset(&server, '0', sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(25100);
  server.sin_addr.s_addr = inet_addr("128.186.120.191");

  // bind address and port to master socket
  if (bind(masterSocket, (struct sockaddr *) &server, sizeof(server)) < 0) {
    printf("Socket bind failed!\n");
    return 0;
  }
  else
    printf("Socket binded!\n");

  // server ready to listen
  if (listen(masterSocket, 5) < 0) {
    printf("Socket listening failed!\n");
    return 0;
  }
  else
    printf("Waiting for new connection!\n");

  int addrlen = sizeof(server);

  // main loop
  while (1) {

    // clear the socket set
    FD_ZERO(&readfds);

    // add master socket to set
    FD_SET(masterSocket, &readfds);
    max_sd = masterSocket;

    // add child sockets to set
    for (i = 0; i < maxClients; i++) {

      // socket descriptor
      sd = clientSocket[i].socket;

      // add socket descriptor to read list
      if (sd > 0)
        FD_SET(sd, &readfds);

      // get highest socket descriptor
      if (sd > max_sd)
        max_sd = sd;
    }

    // wait for activity on one of the sockets
    // wait is indefinite
    activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

    // error checking
    if ((activity < 0) && (errno!=EINTR))
      printf("Select error!\n");

    // handle new connection
    if (FD_ISSET(masterSocket, &readfds)) {

      // variable to store username
      char * user;

      // accept new connection
      if ((newSocket = accept(masterSocket, (struct sockaddr *) &server, (socklen_t *) &addrlen)) < 0) {
        printf("New connection failed!\n");
        return 0;
      }

      // display information about connection
      printf("New Connection!\nSocket FD = %d\nIP Address = %s\nPort = %d\n",
      newSocket, inet_ntoa(server.sin_addr), ntohs(server.sin_port));

      // receive username from client
      if (recv(newSocket, buffer, 1024, 0) < 0) {
        printf("Receiving username failed!\n");
        continue;
      }

      // set username
      user = buffer;
      printf("User: %s\n", user);

      // send welcome message
      char * message = "Welcome!\n";
      if (send(newSocket, message, strlen(message), 0) < 0) {
        printf("Sending welcome message failed!\n");
        continue;
      }

      // add new client to array
      for (i = 0; i < maxClients; i++) {

        // if a position is empty
        if (clientSocket[i].socket == 0) {
          clientSocket[i].socket = newSocket;
          clientSocket[i].username = user;
          printf("Added user to list of clients!\n");
          break;
        }
      }
    }

    // check for client commands
    for (i = 0; i < maxClients; i++) {

      // assign socket descriptor of current client
      sd = clientSocket[i].socket;

      if (FD_ISSET(sd, &readfds)) {

        // receive command from client
        if (recv(sd, buffer, 1024, 0) < 0) {
          printf("Receiving command failed!\n");
          continue;
        }

        // print message
        printf("From Client: %s\n", buffer);

        // parse command
        parseSpace(buffer, command);

        // exit command
        if (strcmp(command[0], "logout") == 0) {

          // get connection information
          getpeername(sd , (struct sockaddr*) &server , (socklen_t *) &addrlen);

          printf("Closing Connection!\nSocket FD = %d\nIP Address = %s\nPort = %d\n",
          sd, inet_ntoa(server.sin_addr), ntohs(server.sin_port));

          // close socket and reset client
          close(sd);
          clientSocket[i].socket = 0;
          clientSocket[i].username = NULL;

          // send closing message
          char * message = "Closing connection!\n";
          if (send(sd, message, strlen(message), 0) < 0)
            printf("Sending message failed!\n");
        }

        // chat command
        else if (strcmp(command[0], "chat") == 0) {

          // search for @ char in second parameter
          char * check;

          // broadcast message
          if ((check = strchr(command[1], '@')) != NULL) {

            // go through all clients
            for (i = 0; i < maxClients; i++) {

              // if client is logged in
              if (clientSocket[i].socket != 0) {

                // concatenate message into single string
                char * message;
                int index = 1;
                while (command[index] != NULL)
                  strcat(message, command[index++]);

                // send message
                if (send(clientSocket[i].socket, message, strlen(message), 0) < 0) {
                  printf("Sending message failed!\n");
                  continue;
                }
              }
            }
          }

          // message to a specific user
          else {

            // get username
            char * user;
            int index = 1;
            while (command[1][index] != NULL)
              strcat(user, command[1][index++]);

            // go through all clients
            for (i = 0; i < maxClients; i++) {

              // if client is logged in
              if (clientSocket[i].username == user) {

                // concatenate message into single string
                char * message;

                // message format: user >> message
                strcat(message, user);
                strcat(message, " >> ");

                index = 2;
                while (command[index] != NULL)
                  strcat(message, command[index++]);

                // send message
                if (send(clientSocket[i].socket, message, strlen(message), 0) < 0) {
                  printf("Sending message failed!\n");
                  continue;
                }
              }
            }
          }

          char * message = "Message sent successfully!\n";
          if (send(sd, message, strlen(message), 0) < 0)
            printf("Sending message failed!\n");
        }

        // error checking: wrong input
        else { 
	  printf("Wrong command!\n"); 
	  continue;
	}		
      }
    }
  }
}

// function parses input by space and stores it in command global array
void parseSpace(char * input, char ** cmd)
{
  // parse the input
  char * separator = " ";
  char * parsed;
  parsed = strtok(input, separator);

  // store parsed input in command
  int index = 0;
  while (parsed != NULL)
  {
    cmd[index] = parsed;
    index++;
    parsed = strtok(NULL, separator);
  }

  // finish command with NULL
  cmd[index] = NULL;
}
