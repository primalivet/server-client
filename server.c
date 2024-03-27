#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_socklen_t.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8080
#define CONN_BACKLOG 10

/**
 * We define a global to store our main socket file descriptor since we've to
 * use it when cleaning up after a cancellation signal, in our case SIGINT
 * (Ctrl+c).
 */
int sockfd;

/**
 * Cancellation signal handler
 */

void sigint_handler(int sig) {
  if (sockfd) {
    if (sig == SIGINT) {
      printf("\nShutting down gracefully\n");
      close(sockfd);
      exit(0);
    }
  }
}

int main() {
  /**
   * Create a Socket to handle connections
   * - By using AF_INET as domain we use IPv4 connections
   * - By using SOCK_STREAM as socket type and "0" as socket protocol we use TCP
   *   connections.
   */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Failed to create socket");
    return 1;
  }

  /**
   * Handle process signals, in our case we handle the SIGINT (user pressed
   * Ctrl+c). In case that happens we want to call our signal handler. Which
   * in turn will close the main socket connection.
   * - This is the reason why we need a global for the sockfd, to be able to
   *   access it in the sigint_handler.
   */
  signal(SIGINT, sigint_handler);

  /**
   * Create a internet socket address (sock/addr/_in)
   * - Also for IPv4 since we use AF_INET
   * - INADDR_ANY makes us accept connections from any address
   */
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(PORT);
  address.sin_addr.s_addr = htonl(INADDR_ANY);

  struct sockaddr *address_p = (struct sockaddr *)&address;

  /**
   * Bind our socket to the address we created
   * - From the start the address is just some data.
   * - And the socket is just a socket living in unamed namespace (in terms of
   *   IPv4 space)
   * - So the bind call acctually connects the IPv4 namespace/interface of the
   *   computer to the socket
   */
  if (bind(sockfd, address_p, sizeof(address)) < 0) {
    perror("Failed to bind socket");
    return 1;
  }

  /**
   * Listen for connections
   * - In C this does not automatically accept connections, the listen call
   *   merely says that we now have a "willingness" to accept connections and
   * that we specified a queue CONN_BACKLOG, for how many connections we keep in
   * the queue
   */
  if (listen(sockfd, CONN_BACKLOG) < 0) {
    perror("Failed to listen on socket");
    return 1;
  }

  /**
   * Setup file descriptors needed for select
   * - master_set holds all the file descriptors we should monitor
   * - FD_ZERO initialized the master_set to be empty
   * - FD_SET adds the sockfd file descriptor to the master set
   * - fd_max keeps track of the highest number file descriptor, at this point
   *   it's our only file descriptors, sockfd
   */
  fd_set master_set;
  FD_ZERO(&master_set);
  FD_SET(sockfd, &master_set);
  int fd_max = sockfd;
  fd_set read_fds;

  printf("Listening on port %d\n", PORT);

  /**
   * Start an infinite loop as we want to monitor for file descriptors forever,
   * until the process is terminated
   */
  while (1) {
    /**
     * Mark the "ready to be read" file descriptors with select
     * - read_fds is a copy of master_set to use in select
     * - select modifies the read_fds to indicate which file descriptors
     *   are ready to be read.
     */
    read_fds = master_set;
    if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("Failed to select a file descriptor");
      exit(EXIT_FAILURE);
    }

    // Iterate over all the file descriptors
    for (int i = 0; i <= fd_max; i++) {
      // Check if the current file descriptor is ready to be read
      if (FD_ISSET(i, &read_fds)) {
        if (i == sockfd) {
          // The listening file descriptor is ready, so there is a new
          // connection
          struct sockaddr_in clientaddr;
          socklen_t addrlen = sizeof(clientaddr);
          struct sockaddr *clientaddr_p = (struct sockaddr *)&clientaddr;
          /**
           * Here we accept the ready connection, so it does not block, since
           * it's ready and therefore we're not waiting
           */
          int newfd = accept(sockfd, clientaddr_p, &addrlen);

          if (newfd == -1) {
            perror("Failed to accept connection");
          } else {
            /**
             * Add the new connection file descriptor to the master_set so we
             * can monitor it
             */
            FD_SET(newfd, &master_set);
            if (newfd > fd_max) {
              // Update the max file descriptor if nessasary
              fd_max = newfd;
            }

            char *dot_notation_ipaddress = inet_ntoa(clientaddr.sin_addr);
            printf("New connection from %s on socket %d\n",
                   dot_notation_ipaddress, newfd);
          }
        } else {
          // If it's not the listening socket, then it's a "data ready" on
          // socket.
          char buffer[BUFFER_SIZE];
          // Initialize the buffer to be empty
          memset(buffer, 0, BUFFER_SIZE);
          // Try to read data from the socket
          int nbytes = read(i, buffer, sizeof(buffer));
          if (nbytes <= 0) {
            /**
             * If nbytes is 0 the socket hung up, otherwise (below zero) we had
             * an error reading
             */
            if (nbytes == 0) {
              printf("Connection closed: Socket %d hung up\n", i);
            } else {
              perror("Unable to read from socket");
            }

            /**
             * Close the socket and remove it from the master_set so that we
             * dont monitor it anymore
             */
            close(i);
            FD_CLR(i, &master_set);
          } else {
            /**
             * Socket was read successfully, process and respond.
             */
            printf("Recived message from socket %d: %s\n", i, buffer);

            char *response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: 18\r\n"
                             "\r\n"
                             "Hello from server!";

            printf("Sending response to socket: %d\n", i);

            /**
              * Write the response to the current socket 
              */
            write(i, response, strlen(response));
          }
        }
      } // close if FD_ISSET
    }   // close for(i < fd_max)
  }     // close while(1)

  return 0;
}
