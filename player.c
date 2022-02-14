#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

#define MAX_LIMIT 100

struct Player {
    int Id;
    int master_fd;
    int leftNeighbor_fd;
    int rightNeighbor_fd;
};


int isValidPortNumber(const char *portNum, int *result);
void setup(const char *machineName, const char *portNum, struct Player *me);
void executeListen(struct Player *me);
void executeConnect(char *message, struct Player *me);
int getId(char *message);

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        exitWithError("The command should be of format: ./player [machine_name] [port_num]");
    }
    const char *machineName = argv[1];
    int portNum;
    if (!isValidPortNumber(argv[2], &portNum)) {
        exitWithError("The port number should be between 0 and 65535");
    }

    struct Player *me = malloc(sizeof(*me));

    // set up connection with ringmaster, left & right neighbors
    setup(machineName, argv[2], me);

    // free all resources
    return 0;
}

void setup(const char *machineName, const char *portNum, struct Player *me) {
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(machineName, portNum, &host_info, &host_info_list);
    if (status != 0) {
       exitWithError("Cannot get address info for host");
    }

    int master_fd = createNewSocket(host_info_list);
    me->master_fd = master_fd;
    
    // connect to ringmaster
    status = connect(master_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        exitWithError("Cannot connect to socket");
    }

    // waiting for command from ringmaster, until "D" -- "Done"
    printf("Waiting for command from ringmaster...\n");
    while (1) {
        char message[MAX_LIMIT];
        recvMessage(master_fd, message, MAX_LIMIT);
        printf("Player received: %s\n", message);

        // if setup done
        if (message[0] == 'D') {
            break;
        }
        // if is ID info
        else if (message[0] == 'I') {
            me->Id = getId(message);
            // send feedback to ringmaster
            sendMessage(me->master_fd, "D");
        }
        // if is requested listening
        else if (message[0] == 'L') {
            executeListen(me);
        } 
        // if is requested connecting
        else if (message[0] == 'C') {
            executeConnect(message, me);
        }
    }

    freeaddrinfo(host_info_list);
}

// connect to the right neighbor
void executeConnect(char *message, struct Player *me) {
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    // parse the connect message
    char *token = strtok(message, " ");
    const char *hostname = strtok(NULL, " ");
    const char *port = strtok(NULL, " ");
    printf("%s\n%s\n", hostname, port);

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
       exitWithError("Cannot get address info for host");
    }

    int rightNeighbor_fd = createNewSocket(host_info_list);
    
    // connect to right neighbor
    status = connect(rightNeighbor_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        exitWithError("Cannot connect to socket");
    }
    me->rightNeighbor_fd = rightNeighbor_fd;
    freeaddrinfo(host_info_list);
}

// connect to the left neighbor
void executeListen(struct Player *me) {
    // first create a socket and listen it
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    char hostname[100];
    status = gethostname(hostname, sizeof(hostname));
    if (status == -1) {
        exitWithError("Cannot get my hostname");
    }

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, NULL, &host_info, &host_info_list);
    if (status != 0) {
       exitWithError("Cannot get address info for host");
    }

    int listen_fd = createNewSocket(host_info_list);
    int yes = 1;
    status = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(listen_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        exitWithError("Cannot bind socket");
    }

    status = listen(listen_fd, 10);
    if (status == -1) {
        exitWithError("Cannot listen on socket");
    }

    // get my portnum and send to ringmaster
    char localHost[20];
    int localPort;
    getLocalHostAndPort(listen_fd, localHost, sizeof(localHost), &localPort);

    // give feedback to ringmaster
    char message[MAX_LIMIT];
    sprintf(message, "L %s %d", localHost, localPort);
    sendMessage(me->master_fd, message);
  
    // waiting for connection by left neighbor
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);

    int leftNeighbor_fd;
    printf("Waiting for left neighbor's connection on port %d\n", localPort);
    leftNeighbor_fd = accept(listen_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (leftNeighbor_fd == -1) {
        exitWithError("Cannot accept connection on socket");
    }

    me->leftNeighbor_fd = leftNeighbor_fd;

    // give feedback to ringmaster
    sendMessage(me->master_fd, "D");

    freeaddrinfo(host_info_list);
    close(listen_fd);
}

int getId(char *message) {
    // Extract the first token
    char *id = strtok(message, " ");
    id = strtok(NULL, " ");
    int Id;
    if (convertToNumber(id, &Id) == 0) {
        exitWithError("The Id is not a valid number");
    }
    return Id;
}

int isValidPortNumber(const char *portNum, int *result) {
    if (!convertToNumber(portNum, result) || *result < 0 || *result > 65535) {
        return 0;
    }
    return 1;
}

