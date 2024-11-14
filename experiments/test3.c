#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <signal.h>
#include <unistd.h>
#include "messages.h"

// NOTE: Some socket logic taken from https://www.tutorialspoint.com/unix_sockets/index.htm


// Socket variables are global so that they can be closed by handler.
int sockfd;
int newsockfd;

// CTRL+C interrupt handler for graceful termination
void terminationHandler(int sig) {
    close(sockfd);
    close(newsockfd);
    exit(0);
}

int main(int argc, char *argv[]) {

    // Set up signal for graceful termination
    signal(SIGINT, terminationHandler);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Setting the port available in case it is not
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    // Initialize socket structure
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    // Bind to host address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // Listen for client
    listen(sockfd, 100);

    // Declare client address and size
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);

    // Begin accepting client connections as concurrent child processes
    while (1) {

        // Accept connection and check for error
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        // Fork off a child process and check for error
        int pid = fork();
        if (pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }

        // Child: process a request and return a result
        if (pid == 0) {

            // Close the original socket on this process
            close(sockfd);

            // Read in request through new socket
            char buffer[PACKET_REQUEST_SIZE];
            bzero(buffer, PACKET_REQUEST_SIZE);
            read(newsockfd, buffer, PACKET_REQUEST_SIZE);

            // Declare request components
            uint8_t hash[32];
            uint64_t start;
            uint64_t end;
            uint8_t p;

            // Extract components from request
            memcpy(hash, buffer + PACKET_REQUEST_HASH_OFFSET, 32);
            memcpy(&start, buffer + PACKET_REQUEST_START_OFFSET, 8);
            memcpy(&end, buffer + PACKET_REQUEST_END_OFFSET, 8);
            memcpy(&p, buffer + PACKET_REQUEST_PRIO_OFFSET, 1);

            // Set process priority
            nice(16 - p);

            // Convert byte order as needed
            start = htobe64(start);
            end = htobe64(end);

            // Search for key in given range corresponding to given hash
            uint8_t calculatedHash[32];
            uint64_t key;
            for (key = start; key < end; key++) {
                SHA256((uint8_t *)&key, 8, calculatedHash);
                if (memcmp(hash, calculatedHash, 32) == 0)
                    break;
            }

            // Send resulting key back to client
            key = be64toh(key);
            write(newsockfd, &key, 8);

            // Clean up and exit the child process
            close(newsockfd);
            exit(0);
        }

        // Parent: close the new socket, then begin loop again
        else {
            close(newsockfd);
        }
    }

    return 0;
}