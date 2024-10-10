#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <signal.h>

#include "code/messages.h"

// NOTE: Used https://www.tutorialspoint.com/unix_sockets/client_server_model.htm to understand and build our socket logic


// Global socket variables to be used by handler.
int server_fd;
int client_socket;

// CTRL+C termination / interrupt handler 
void terminationHandler(int sig) {
    close(server_fd);
    close(client_socket);
    exit(0);
}

int main(int argc, char *argv[]) {

    // Initialize termination signal 
    signal(SIGINT, terminationHandler);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Make sure the port is available 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    // Initialize socket structure and bind the socket to the port
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    // Listen for client
    listen(server_fd, 100);

    // Declare client address and size
    struct sockaddr_in cli_addr;
    int cli_lengt = sizeof(cli_addr);

    // Accept client connections as concurrent child processes
    while (1) {

        // Accept and check for error
        client_socket = accept(server_fd, (struct sockaddr *) &cli_addr, &cli_lengt);
        if (client_socket < 0) {
            perror("Accept failed");
            exit(1);
        }

        // Fork off a child process and check for error
        int pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        // Child process
        if (pid == 0) {

            // Close the original socket on this process
            close(server_fd);

            // Read in request through client socket
            char buffer[PACKET_REQUEST_SIZE];
            bzero(buffer, PACKET_REQUEST_SIZE);
            read(client_socket, buffer, PACKET_REQUEST_SIZE);

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

            // Convert byte order as needed
            start = htobe64(start);
            end = htobe64(end);

            // Search for key in given range corresponding to given hash
            uint8_t calculatedHash[32];
            uint64_t key;
            for (uint64_t i = start; i < end; i++) {
                SHA256_CTX sha256;
                SHA256_Init(&sha256);
                SHA256_Update(&sha256, &i, 8);
                SHA256_Final(calculatedHash, &sha256);
                if (memcmp(hash, calculatedHash, 32) == 0) {
                    key = i;
                    break;
                }
            }

            // Send back found key to client
            key = be64toh(key);
            write(client_socket, &key, 8);

            // Close the child process
            close(client_socket);
            exit(0);
        }

        // Close client socket and begin loop again
        else {
            close(client_socket);
        }
    }

    return 0;
}