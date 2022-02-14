#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


// if not a valid number, 0 would be returned
// if is a valid number, would put it in the result
int convertToNumber(const char *str, int *result);
void exitWithError(const char *msg);
void recvMessage(int socket_fd, char *message, int max_limit);
void sendMessage(int socket_fd, const char *message);
void getHostAndPort(int socket_fd, char *hostname, socklen_t len, int *port);
void getLocalHostAndPort(int socket_fd, char *hostname, socklen_t len, int *port);
int createNewSocket(struct addrinfo *host_info_list);

#endif //UTILS_H