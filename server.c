#include <netinet/in.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "code/messages.h"
#include "code/cache.h"
#include "code/priorityQ.h"

// Semaphore to track the number of requests in the queue
sem_t requests_in_queue;
// Mutex to protect shared resources
pthread_mutex_t lock;

// Global cache and priority queue for requests
cache_t cache;
priorityQ_t pq;

// Structure for the thread pool
typedef struct {
    pthread_t* threads; // Array of worker threads
    int thread_count;    // Number of threads in the pool
    int stop;           // Flag to stop the threads
} thread_pool_t;

// Worker thread function to process requests
void* worker_thread(void* arg) {
    thread_pool_t* pool = (thread_pool_t*)arg;
    while (1) {
        // Wait for a request to be available in the queue
        sem_wait(&requests_in_queue);

        // Lock the mutex to safely access the shared priority queue
        pthread_mutex_lock(&lock);
        // Extract a request from the priority queue
        request_t request = extract(&pq);
        pthread_mutex_unlock(&lock);

        // Calculate the hash for the range specified in the request
        uint8_t calculatedHash[32];
        uint64_t key;
        for (key = request.start; key < request.end; key++) {
            SHA256((uint8_t *)&key, 8, calculatedHash);
            if (memcmp(request.hash, calculatedHash, 32) == 0)
                break;
        }

        // Convert the key to network byte order and send it back to the client
        key = be64toh(key);
        write(request.newsockfd, &key, 8);
        close(request.newsockfd);

        // Insert the calculated hash and key into the cache
        pthread_mutex_lock(&lock);
        cache_insert(&cache, calculatedHash, key);
        pthread_mutex_unlock(&lock);
    }
}

// Initialize the thread pool
void init_thread_pool(thread_pool_t* pool, int thread_count) {
    pool->thread_count = thread_count;
    pool->stop = 0;
    pool->threads = malloc(sizeof(pthread_t) * thread_count);
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }
}

// Clean up the thread pool
void destroy_thread_pool(thread_pool_t* pool) {
    pool->stop = 1;
    for (int i = 0; i < pool->thread_count; i++) {
        sem_post(&requests_in_queue); // Wake up threads to exit
    }
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    free(pool->threads);
}

// Start server
int main(int argc, char *argv[]) {
    // Check for the correct number of arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // Initialize Mutex
    pthread_mutex_init(&lock, NULL);

    // Initialize shared request priority queue and cache
    init_priorityQ(&pq);
    init_cache(&cache);

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Make sure the port is available
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

    // Listen for client
    listen(sockfd, 100);

    // Declare client address and size
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);

    // Create thread pool
    thread_pool_t pool;
    init_thread_pool(&pool, 7); // Initialize with 7 threads

    // Counter to track the number of requests
    int requestCounter = 0;

    // Main loop to accept incoming connections
    while (1) {
        // Accept a new client connection and check for error
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }
        requestCounter++; // Increment the request counter

        // Buffer to hold the incoming request
        uint8_t buffer[PACKET_REQUEST_SIZE];
        // Read the request from the client
        read(newsockfd, buffer, PACKET_REQUEST_SIZE);

        // Variables to hold the extracted request data
        uint8_t hash[32];
        uint64_t start;
        uint64_t end;
        uint8_t p;

        // Extract the hash, start, end, and priority from the request buffer
        memcpy(hash, buffer + PACKET_REQUEST_HASH_OFFSET, 32);
        memcpy(&start, buffer + PACKET_REQUEST_START_OFFSET, 8);
        memcpy(&end, buffer + PACKET_REQUEST_END_OFFSET, 8);
        memcpy(&p, buffer + PACKET_REQUEST_PRIO_OFFSET, 1);

        // Convert start and end byte order
        start = htobe64(start);
        end = htobe64(end);

        // Lock the mutex to check the cache for existing results
        pthread_mutex_lock(&lock);
        pair_t *cache_result = cache_search(&cache, hash);
        pthread_mutex_unlock(&lock);

        // If a result is found in the cache, send it back to the client
        if (cache_result) {
            write(newsockfd, &cache_result->key, 8);
            close(newsockfd);
            continue;
        }

        // Initialize a new request structure
        request_t request;
        init_request(&request, hash, start, end, p, newsockfd, requestCounter);

        // Lock the mutex to safely insert the new request into the priority queue
        pthread_mutex_lock(&lock);
        insert(&pq, &request);
        pthread_mutex_unlock(&lock);

        // Signal that a new request is available for processing
        sem_post(&requests_in_queue);
    }

    // Clean up
    destroy_thread_pool(&pool);
    close(sockfd);
    return 0;
}
