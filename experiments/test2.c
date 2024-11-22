#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <signal.h>
#include <pthread.h>

#include "code/messages.h"
#include "cache_array.h" // Include the cache header

// NOTE: Used https://www.tutorialspoint.com/unix_sockets/client_server_model.htm to understand and build our socket logic

// Function to handle the reverse hashing process
void* reverseHash(void *newsockfdPtr) {

    // Retrieve the socket file descriptor from the pointer
    int newsockfd = *(int*)newsockfdPtr;
    free(newsockfdPtr); // Free the allocated memory for the socket pointer

    // Buffer to hold the incoming request
    uint8_t buffer[PACKET_REQUEST_SIZE];
    read(newsockfd, buffer, PACKET_REQUEST_SIZE); // Read the request from the client

    // Variables to hold the extracted request data
    uint8_t hash[32];
    uint64_t start;
    uint64_t end;
    uint8_t p;
    memcpy(hash, buffer + PACKET_REQUEST_HASH_OFFSET, 32);
    memcpy(&start, buffer + PACKET_REQUEST_START_OFFSET, 8);
    memcpy(&end, buffer + PACKET_REQUEST_END_OFFSET, 8);
    memcpy(&p, buffer + PACKET_REQUEST_PRIO_OFFSET, 1);

    // Convert start and end byte order
    start = htobe64(start);
    end = htobe64(end);

    // Initialize the cache (this should be done once in the main function)
    static cache_t cache;
    static int cache_initialized = 0;
    if (!cache_initialized) {
        init_cache(&cache); // Initialize the cache
        cache_initialized = 1; // Set the flag to indicate cache is initialized
    }

    // Check if the result is already in the cache
    pair_t *cache_result = cache_search(&cache, hash);
    if (cache_result) {
        // If found in cache, send the key back to the client
        uint64_t key = cache_result->key;
        key = be64toh(key); // Convert key to network byte order
        write(newsockfd, &key, 8);
    } else {
        // If not found in cache, calculate the hash
        uint8_t calculatedHash[32];
        uint64_t key;
        for (key = start; key < end; key++) {
            SHA256((uint8_t *)&key, 8, calculatedHash);
            if (memcmp(hash, calculatedHash, 32) == 0)
                break; // Found the matching hash
        }

        // Convert the key to network byte order and send it back to the client
        key = be64toh(key);
        write(newsockfd, &key, 8);

        // Insert the calculated hash and key into the cache
        cache_insert(&cache, calculatedHash, key);
    }

    // Close the socket connection
    close(newsockfd);
    pthread_exit(NULL); // Exit the thread
}

// Main function to start the server
int main(int argc, char *argv[]) {
    // Check for the correct number of arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // Set socket options to reuse the address
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    // Initialize socket structure and bind the socket to the port
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    // Bind to host address, and check for error
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // Listen for client connections
    listen(sockfd, 100);

    // Declare client address and size
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);

    // Main loop to accept incoming connections
    while (1) {
        // Accept a new client connection and check for error
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        // Allocate memory for the socket file descriptor
        int *newsockfdPtr = malloc(sizeof(int));
        memcpy(newsockfdPtr, &newsockfd, sizeof(int)); // Copy the socket file descriptor

        // Create a new thread to handle the request
        pthread_t tid;
        pthread_create(&tid, NULL, reverseHash, newsockfdPtr); // Start the reverseHash function in a new thread
    }

    // Close the server socket (this line will never be reached in the current infinite loop)
    close(sockfd);
    return 0; // Return success
}