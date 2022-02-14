#include "utils.h"

#define MAX_LIMIT 100

// if not a valid number, 0 would be returned
// if is a valid number, would put it in the result
int convertToNumber(const char *str, int *result){
    char *rest = NULL;
    long num = strtol(str, &rest, 10);
    if (*rest != '\0') {
        return 0;
    }
    *result = num;
    return 1;
}

void exitWithError(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

void recvMessage(int socket_fd, char *message, int max_limit) {
    ssize_t recv_size = recv(socket_fd, message, max_limit - 1, 0);

    if (recv_size == -1) {
        exitWithError("Receive message failed");
    } else if (recv_size == 0) {
        exitWithError("The ringmaster shut down");
    }

    message[recv_size] = '\0';
}

void sendMessage(int socket_fd, const char *message) {
    int status = send(socket_fd, message, strlen(message), 0);
    if (status == -1) {
        exitWithError("Send message failed");
    }
}

void getLocalHostAndPort(int socket_fd, char *hostname, socklen_t len, int *port) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    int status = getsockname(socket_fd, (struct sockaddr *)&addr, &addr_size);
    if (status == -1) {
        exitWithError("Cannot get local socket info");
    }
    if (inet_ntop(AF_INET, &addr.sin_addr, hostname, len) == NULL) {
        exitWithError("Invalid ip");
    }
    *port = htons(addr.sin_port);
}

void getHostAndPort(int socket_fd, char *hostname, socklen_t len, int *port) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int status = getpeername(socket_fd, (struct sockaddr *)&addr, &addr_size);
    if (status == -1) {
        exitWithError("Cannot get peer info");
    }
    if (inet_ntop(AF_INET, &addr.sin_addr, hostname, len) == NULL) {
        exitWithError("Invalid ip");
    }
    *port = htons(addr.sin_port);
}

int createNewSocket(struct addrinfo *host_info_list) {
    int socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (socket_fd == -1) {
        exitWithError("Cannot create socket");
    }
    return socket_fd;
}