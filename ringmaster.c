#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include "utils.h"

#define BACKLOG 100
#define MAX_LIMIT 3000

struct _InitialInfo {
    int Id;
    const char *hostname;
    const char *port;
};
typedef struct _InitialInfo InitialInfo;

int isValidPortNumber(const char *portNum, int *result);
int isValidNumPlayers(const char *numPlayers, int *result);
int isValidNumHops(const char *numHops, int *result);
int setup(const char *hostname, const char *port);
void waitPlayers(int numPlayers, int main_socket_fd, int *clientFds);
void freeFds(int *clientFds, int numPlayers);
void connectPlayers(int numPlayers, int *clientFds);
void waitPotato(int *clientFds, int numPlayers, char *potato);
int maxFd(int *clientFds, int numPlayers);
void displayResult(char *potato);

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        exitWithError("The command should be of format: ./ringmaster [port] [num_players] [num_hops]");
    }

    // check validity and convert to int
    int portNum;
    int numPlayers;
    int numHops;
    if (!isValidNumHops(argv[3], &numHops) || !isValidNumPlayers(argv[2], &numPlayers)
         || !isValidPortNumber(argv[1], &portNum)) {
        exitWithError("The port number should be between 0 and 65535\n\
The number of players should be greater than 0\n\
The number of hops should be between 0 and 512");
    }

    // printf("%d\n%d\n%d\n", portNum, numPlayers, numHops);
    printf("Potato Ringmaster\n");
    printf("Players = %d\n", numPlayers);
    printf("Hops = %d\n", numHops);

    // setup the ringmaster side socket, used for initial connection
    int *clientFds = malloc(sizeof(*clientFds) * numPlayers);
    int main_socket_fd = setup(NULL, argv[1]);
    printf("Setup done.\n");

    // connect to the players
    // wait until all the players are ready
    waitPlayers(numPlayers, main_socket_fd, clientFds);
    printf("Players all show up\n");

    // manage to connect all the players as a ring
    connectPlayers(numPlayers, clientFds);

    // randomly pick a player to start the game
    srand((unsigned int)time(NULL));
    int initPlayer = rand() % numPlayers;

    // create the potato
    char potato[MAX_LIMIT] = {0};
    sprintf(potato, "%d", numHops);

    // sending the potato to the first player
    printf("Ready to start the game, sending potato to player %d\n", initPlayer);
    sendMessage(clientFds[initPlayer], potato);

    // waiting for the potato to come back
    memset(potato, 0, sizeof(potato));
    waitPotato(clientFds, numPlayers, potato);

    // display results
    displayResult(potato);

    // free all the resources
    close(main_socket_fd);
    freeFds(clientFds, numPlayers);
    return 0;
}

// Trace of potato:
// 2,1,2,0,2,1,0,2,...
void displayResult(char *potato) {
    printf("Trace of potato:\n");
    // Extract the first token
    char *token = strtok(potato, " ");
    // start from the second token
    token = strtok(NULL, " ");
    printf("%s", token);
    token = strtok(NULL, " ");
    // loop through the string to extract all other tokens
    while(token != NULL) {
        printf(",%s", token);
        token = strtok(NULL, " ");
    }
    printf("\n");
}

void waitPotato(int *clientFds, int numPlayers, char *potato) {
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // add the players to the master set
    for (int i = 0; i < numPlayers; i++) {
        FD_SET(clientFds[i], &master);
    }

    // keep track of the biggest file descriptor
    int fdmax = maxFd(clientFds, numPlayers);

    // waiting for hot potato
    while(1) {
        read_fds = master; // copy it
        int status = select(fdmax+1, &read_fds, NULL, NULL, NULL);
        if (status == -1) {
            exitWithError("Select failed");
        }
        for (int i = 0; i < numPlayers; i++) {
            if (FD_ISSET(clientFds[i], &read_fds)) {
                recvMessage(clientFds[i], potato, MAX_LIMIT);
                if (strlen(potato) == 0) {
                    exitWithError("One player left");
                }
                return;
            }
        }
    }
}

int maxFd(int *clientFds, int numPlayers) {
    int maxFd = clientFds[0];
    for (int i = 1; i < numPlayers; i++) {
        maxFd = maxFd > clientFds[i] ? maxFd : clientFds[i];
    }
    return maxFd;
}

// manage to connect all the players as a ring
void connectPlayers(int numPlayers, int *clientFds) {
    for (int i = 0; i < numPlayers; i++) {
        // let player i + 1 listening for connection
        sendMessage(clientFds[(i + 1) % numPlayers], "L");
        // wait for feedback
        char message[MAX_LIMIT];
        recvMessage(clientFds[(i + 1) % numPlayers], message, MAX_LIMIT);
        printf("%s\n", message);
        
        // let player i to connect to it
        message[0] = 'C';
        sendMessage(clientFds[i], message);
        // wait for feedback
        memset(message, 0, sizeof(message));
        recvMessage(clientFds[(i + 1) % numPlayers], message, sizeof(message));
        assert(message[0] == 'D');

        printf("Player %d connected to its neighbor Player %d\n", i, (i + 1) % numPlayers);
    }

    printf("All players have formed a ring.\n");
    // inform all the players the setup is done
    for (int i = 0; i < numPlayers; i++) {
        sendMessage(clientFds[i], "D");
        // receive feedback from player
        char message[MAX_LIMIT] = {0};
        recvMessage(clientFds[i], message, sizeof(message));
        assert(message[0] == 'D');

        printf("Player %d is ready to play\n", i);
    }
}

// wait until all the players are ready
void waitPlayers(int numPlayers, int main_socket_fd, int *clientFds) {
    printf("Waiting for connections...\n");
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    
    int id = 0;
    while (id != numPlayers) {
        printf("Waiting for player %d...\n", id);
        int client_connection_fd;
        client_connection_fd = accept(main_socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        if (client_connection_fd == -1) {
            exitWithError("Cannot accept connection on socket");
        }

        // assign the id to the client
        clientFds[id] = client_connection_fd;
        char message[MAX_LIMIT];
        sprintf(message, "I %d", id);
        sendMessage(client_connection_fd, message);

        // recv feedback from players
        memset(message, 0, sizeof(message));
        recvMessage(client_connection_fd, message, sizeof(message));
        assert(message[0] == 'D');

        ++id;
    }
}

// setup the main socket, return the main socket fd
int setup(const char *hostname, const char *port) {
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        exitWithError("Cannot get address info for host");
    }
    socket_fd = createNewSocket(host_info_list);

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (socket_fd == -1) {
        exitWithError("Cannot bind socket");
    }

    status = listen(socket_fd, BACKLOG);
    if (status == -1) {
        exitWithError("Cannot listen on socket");
    }
  
    freeaddrinfo(host_info_list);
    return socket_fd;
}

// port number should be 0~65535
int isValidPortNumber(const char *portNum, int *result) {
    if (!convertToNumber(portNum, result) || *result < 0 || *result > 65535) {
        return 0;
    }
    return 1;
}

// should be > 1
int isValidNumPlayers(const char *numPlayers, int *result) {
    if (!convertToNumber(numPlayers, result) || *result <= 1) {
        return 0;
    }
    return 1;
}

// should be >= 0 & <= 512
int isValidNumHops(const char *numHops, int *result) {
    if (!convertToNumber(numHops, result) || *result < 0 || *result > 512) {
        return 0;
    }
    return 1;
}

void freeFds(int *clientFds, int numPlayers) {
    for (int i = 0; i < numPlayers; i++) {
        close(clientFds[i]);
    }
    free(clientFds);
}