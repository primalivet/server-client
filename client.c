#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
  /**
   * Create a Socket to handle connections
   * - By using AF_INET as domain we use IPv4 connections
   * - By using SOCK_STREAM as socket type and "0" as socket protocol we use TCP
   *   connections.
   */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Failed to create socket\n");
    return 1;
  }

  /**
   * Create a internet socket address (sock/addr/_in)
   * - Also for IPv4 since we use AF_INET
   */
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVER_PORT);

  /**
   * Create a binary representation of the regular "ip address string" and
   * assign it to the address field of the server_adress structure
   */
  if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
    perror("Failed to convert server ip address to binary\n");
    return 1;
  }

  struct sockaddr *server_address_p = (struct sockaddr *)&server_address;

  /**
   * Connect our socket to the server address
   */
  if (connect(sockfd, server_address_p, sizeof(server_address)) < 0) {
    perror("Failed to connect to server\n");
    exit(1);
  }

  /**
   * Create a static HTTP request
   */
  char *request_mesage = "GET / HTTP/1.1\r\n"
                         "Content-Type: text/plain\r\n"
                         "\r\n";

  /**
   * Write and send the request to the socket
   */
  if (send(sockfd, request_mesage, strlen(request_mesage), 0) < 0) {
    perror("Failed to send message\n");
    exit(1);
  }

  /**
   * Create a buffer to read from
   * - We also "clear" the buffer so that there isn't any garbage in memory
   */
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);

  /**
   * Read the response from the socket into our buffer
   */
  if (recv(sockfd, buffer, BUFFER_SIZE - 1, 0) < 0) {
    perror("Failed to read response\n");
  }

  printf("Server replied: %s", buffer);

  /**
   * Close the socket before exiting the program
   */
  close(sockfd);

  return 0;
}
