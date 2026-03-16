#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "code/messages.h"
#include "code/priorityQ.h"

// NOTE: Used https://www.tutorialspoint.com/unix_sockets/client_server_model.htm to understand and build our socket logic

// Declare semaphore and mutex
sem_t requests_in_queue;
pthread_mutex_t lock;

// Takes in pointer to priority queue
void* worker_thread(void *pq) {

    // Repeatedly take requests from queue and process them
    while (1) {

        // Block while queue is empty
        sem_wait(&requests_in_queue);

        // Extract the request with the highest priority
        pthread_mutex_lock(&lock);
        request_t request = extract(pq);
        pthread_mutex_unlock(&lock);

        // Search for key in given range corresponding to given hash
        uint8_t calculatedHash[32];
        uint64_t key;
        for (key = request.start; key < request.end; key++) {
            SHA256((uint8_t *)&key, 8, calculatedHash);
            if (memcmp(request.hash, calculatedHash, 32) == 0)
                break;
        }

        // Send resulting key back to client and close socket
        key = be64toh(key);
        write(request.newsockfd, &key, 8);
        close(request.newsockfd);
    }
}

// Start server
int main(int argc, char *argv[]) {

    // Initialize Mutex
    pthread_mutex_init(&lock, NULL);

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Set the port as available in case it is not available, and check for error
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

    // Bind to host address, and check for error
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // Listen for client
    listen(sockfd, 100);

    // Prepare client address and size
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);

    // Initialize the priority queue
    priorityQ_t pq;
    init_priority_queue(&pq);

    // Initialize thread pool
    pthread_t threads[4];
    for (int i = 0; i < 4; i++)
        pthread_create(&threads[i], NULL, worker_thread, &pq);

    // Initialize request counter
    int c = 0;

    // Continuously accept requests to populate the priority queue
    while (1) {

        // Accept connection, and check for error
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        // Read in request through new connection
        uint8_t buffer[PACKET_REQUEST_SIZE];
        read(newsockfd, buffer, PACKET_REQUEST_SIZE);

        // Extract components from request
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

        // Create request object
        request_t request;
        init_request(&request, hash, start, end, p, newsockfd, ++c);

        // Insert request into priority queue
        pthread_mutex_lock(&lock);
        insert(&pq, &request);
        pthread_mutex_unlock(&lock);

        // Up the number of requests in queue
        sem_post(&requests_in_queue);
    }
}