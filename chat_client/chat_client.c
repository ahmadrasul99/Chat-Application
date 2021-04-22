#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// global variables
pthread_t tid; // intialize thread
int clientSocket; // socket variable
char * command[100]; // stores current command

// function declarations
void parseSpace(char *, char **);
void * processConnection(void *);

// handling signals
void signalHandler(int signum)
{
  printf("Signal caught, closing opened socket and/or thread!\n");
  // close socket
  if (clientSocket)
    close(clientSocket);
  // close thread
  if (tid)
    pthread_exit(NULL);
  // end program
  exit(0);
}

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

  /* STARTING CLIENT CODE */

  // signal handler
  signal(SIGINT, signalHandler);

  char * input; // take input from user

  // welcome message
  printf("Welcome to the Client!\n");

  // main loop
  while (1) {
    // take input
    input = readline("$ ");

    // parse input
    parseSpace(input, command);

    // exit command
    if (strcmp(command[0], "exit") == 0) { break; }

    // login command
    else if (strcmp(command[0], "login") == 0) {
      // error checking: no username provided
      if (command[1] == NULL) {
        printf("No username passed!\n");
        continue;
      }

      // generate thread to handle connection
      if (pthread_create(&tid, NULL, &processConnection, NULL) != 0)
        printf("Thread failed!\n");
      else
        printf("Thread created!\n");

      // wait on thread
      pthread_join(tid, NULL);
    }

    // error checking: wrong input
    else { printf("Wrong command!\n"); }
  }

  // closing message
  printf("Closing the Client!\n");
  return 0;
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

// function processes connection upon login command
void * processConnection(void * arg)
{
  // buffer to read in from server
  char buffer[1024];

  // create socket
  clientSocket = socket(AF_INET, SOCK_STREAM, 0);

  // error checking: socket creation failed
  if (clientSocket < 0) {
    printf("Socket creation failed!\n");
    return  0;
  }
  else
    printf("Socket created!\n");

  // initialize server structure
  struct sockaddr_in server;
  memset(&server, '0', sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(7799);
  server.sin_addr.s_addr = inet_addr("localhost");

  // connect to server
  int in = connect(clientSocket, (struct sockaddr *) &server, sizeof(server));

  // error checking: connection failed
  if (in < 0) {
    printf("Connection failed!\n");
    return 0;
  }
  else
    printf("Connection created!\n");

  // send username to server
  in = send(clientSocket, command[1], strlen(command[1]), 0);

  // error checking: writing to server failed
  if (in < 0) {
    printf("Writing to server failed!\n");
    return 0;
  }

  // receive welcome message
  in = recv(clientSocket, buffer, 1024, 0);

  // error checking: reading from server failed
  if (in < 0) {
    printf("Reading from server failed!\n");
    return 0;
  }

  printf("From Server: %s\n", buffer);

  char * input; // take input from user

  // loop for sending commands to server
  while (1) {

    // take input
    input = readline("To Server: ");

    // parse input
    parseSpace(input, command);

    // exit command
    if (strcmp(command[0], "logout") == 0) {

      // send command to server
      in = send(clientSocket, input, strlen(input), 0);

      // error checking: writing to server failed
      if (in < 0)
        printf("Writing to server failed!\n");

      // receive welcome message
      in = recv(clientSocket, buffer, 1024, 0);

      // error checking: reading from server failed
      if (in < 0)
        printf("Reading from server failed!\n");

      printf("From Server: %s\n", buffer);

      break;
    }

    // chat command
    else if (strcmp(command[0], "chat") == 0) {

      /// send command to server
      in = send(clientSocket, input, strlen(input), 0);

      // error checking: writing to server failed
      if (in < 0) {
        printf("Writing to server failed!\n");
        return 0;
      }

      // receive welcome message
      in = recv(clientSocket, buffer, 1024, 0);

      // error checking: reading from server failed
      if (in < 0) {
        printf("Reading from server failed!\n");
        return 0;
      }

      printf("From Server: %s\n", buffer);
    }

    // error checking: wrong input
    else { printf("Wrong command!\n"); }
  }

  // closing socket and thread
  printf("Closing connection!\n");
  close(clientSocket);
  pthread_exit(NULL);
}
