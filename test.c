#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int convertToNumber(const char *str, int *result){
    char *rest = NULL;
    long num = strtol(str, &rest, 10);
    if (*rest != '\0') {
        return 0;
    }
    *result = num;
    return 1;
}

int getId(char *message) {
    char *id = strtok(message, " ");
    id = strtok(NULL, " ");
    int Id;
    if (convertToNumber(id, &Id) == 0) {
        exit(EXIT_FAILURE);
    }
    return Id;
}

int main(int argc, char const *argv[]) {
    // char string[50] = "Hello! We are learning about strtok";
    // // Extract the first token
    // char * token = strtok(string, " ");
    // // loop through the string to extract all other tokens
    // while( token != NULL ) {
    //     printf( " %s\n", token ); //printing each token
    //     token = strtok(NULL, " ");
    // }
    // char hostname[100];
    // int status = gethostname(hostname, sizeof(hostname));
    // if (status == -1) {
    //     return -1;
    // }
    // printf("%s\n", hostname);

    // char message[1000] = {"233 0 2 1 3 0"};
    // char id[20];
    // sprintf(id, " %d", 3);
    // strcat(message, id);
    // printf("%s\n", message);
    // printf("%zu\n", sizeof(message));

    // printf("%d\n", atoi("023"));

    char message[1000] = {"005 1 2 1 2 0 1 0"};
    printf("%s\n", message);
    char *hops = strtok(message, " ");
    int hopsLength = strlen(hops);
    int hopsNum = atoi(hops);

    // decrement the hops
    int i = hopsLength - 1;
    while (message[i] == '0') {
        message[i] = '9';
        --i;
    }
    --message[i];
    message[hopsLength] = ' ';
    printf("%s\n", message);
    return 0;
}
