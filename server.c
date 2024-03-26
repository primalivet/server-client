#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    perror("Failed to listen");
    return 1;
  }

  printf("Listening on port %d\n", PORT);

  /**
   * Create a infinite loop to accept connections
   */
  while (1) {
    /**
     * Accept a connection
     * - This call will block until a connection is made
     * - This call returns us the file descriptor for the connection, so we can
     *   use that to acctually read and write on this specific connection.
     */
    int conn_sockfd = accept(sockfd, NULL, NULL);
    if (conn_sockfd < 0) {
      perror("Failed to accept connection");
      continue;
    }

    /**
     * Create a buffer to read from
     * - We also "clear" the buffer so that there isn't any garbage in memory
     * from eariler connections
     */
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    /**
     * Read the content of the connection into the buffer
     */
    ssize_t bytes_read = read(conn_sockfd, buffer, BUFFER_SIZE);
    if (bytes_read < 0) {
      perror("Failed to read conn_sockfd");
      close(conn_sockfd);
    }

    printf("Read %ld bytes: %s\n", bytes_read, buffer);

    /**
     * Create a static HTTP response
     */
    char *response_message = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: 18\r\n"
                             "\r\n"
                             "Hello from server!";
    /**
     * Write the response to the connection socket
     */
    ssize_t bytes_written =
        write(conn_sockfd, response_message, strlen(response_message));
    if (bytes_written < 0) {
      perror("Failed to write conn_sockfd");
    }

    /**
     * Clean up the sockets used
     */
    close(conn_sockfd);
  }

  return 0;
}
