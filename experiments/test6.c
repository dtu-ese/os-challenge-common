#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "messages.h"

int sockfd;
int requestCounter;
int requestsLeft;
int p = 0;
int thread_count;
int processed_requests;

// arrays to store info on each request - each column represents each priority level (1-16)
typedef uint8_t hash_t[32];
int priorityArray[16][1001] = {0};
uint64_t startArray[16][1001] = {0};
uint64_t endArray[16][1001] = {0};
hash_t hashArray[16][1001] = {0};


void* reader(void *arg){
    free(arg);
    int newSockFd;
        int i;  // i is the priority level (-1) being looped over --> note that the loop decrements as higher priorities are addressed first
        i = 15;
        while (i > -1) {
            int j;                      // j is the next spot to look at to be processed
            j = priorityArray[i][0] -
                1;  // decrement by one as we want the last full spot (not the next available spot)
            while (j > 0) {
                // work on request in place priorityArray[i][priorityArray[i][0]-1]
                // Convert byte order as needed
                newSockFd = priorityArray[i][j];

                // only work on request if it hasn't been worked on yet
                if (newSockFd != -20) {
                    uint64_t start = htobe64(startArray[i][j]);
                    uint64_t end = htobe64(endArray[i][j]);
                    hash_t hash;
                    memcpy(hash, hashArray[i][j], 32);
                    priorityArray[i][j] = -20;                  // set newSockFd to -20 if response already sent back

                    uint8_t calculatedHash[32];
                    uint64_t key;

                    for (key = start; key < end; key++) {
                        SHA256((uint8_t * ) & key, 8, calculatedHash);
                        if (memcmp(hash, calculatedHash, 32) == 0)
                            break;
                    }

                    key = be64toh(key);

                    write(newSockFd, &key, 8);                  // send result back to client
                    close(newSockFd);
                }
                j = j - 1;
            }
            i = i - 1;
        }
    thread_count = thread_count -1;
    close(newSockFd);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    processed_requests = 0;
    thread_count = 0;
    // the first element in each row is a count of the next place to be filled in the matrix for each given priority
    int num;
    num = 0;
    while (num <= 15) {
    priorityArray[num][0] = 1;  // setting each value to 1 as this will be the first available spot
    num++;
    }

    requestCounter = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);           // Create socket --> holds the return of the socket function

    // Set the port as available in case it is not available, and check for error
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    // Initialize socket structure
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    // Bind to host address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR when accepting");
        exit(1);
    }

    listen(sockfd, 1000);        // Listen for client --> waits for client to make connection with server

    // Prepare client address and size
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);

    // Accept client connections until there are none left
    while (1) {
            int newSockFd;
            newSockFd = accept(sockfd, (struct sockaddr *) &cli_addr,
                               &clilen); // Accept connection ( will take the first in the queue)
            requestCounter++;

            // check for error
            if (newSockFd < 0) {
                perror("ERROR on accept");
                exit(1); // exit when the newSockFd < 0 as no more requests
            }

            // Read in request through new socket
            uint8_t buffer[PACKET_REQUEST_SIZE];
            read(newSockFd, buffer, PACKET_REQUEST_SIZE);

            // Declare request components
            hash_t hash;
            uint64_t start;
            uint64_t end;
            uint8_t p;

            // Extract components from request
            memcpy(hash, buffer + PACKET_REQUEST_HASH_OFFSET, 32);
            memcpy(&start, buffer + PACKET_REQUEST_START_OFFSET, 8);
            memcpy(&end, buffer + PACKET_REQUEST_END_OFFSET, 8);
            memcpy(&p, buffer + PACKET_REQUEST_PRIO_OFFSET, 1);

            int arraySpot = priorityArray[p - 1][0];                    // find spot in array for this request
            priorityArray[p - 1][0] = priorityArray[p - 1][0] + 1;      // increment count for next available spot
            memcpy(hashArray[p - 1][arraySpot], hash, 32);
            startArray[p - 1][arraySpot] = start;                       // store start value
            endArray[p - 1][arraySpot] = end;                           // store end value
            priorityArray[p - 1][arraySpot] = newSockFd;                // indicate there is a request with != NULL


        // continue threads while there are still requests to be answered
        p++;
        pthread_t tid;
        pthread_create(&tid, NULL, reader, NULL);
    }
}