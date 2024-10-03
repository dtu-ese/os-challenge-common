#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <string.h>
#include "messages.h"

#define QUEUE_SIZE 500
#define THREAD_POOL_SIZE 4

// Request structure for the queue
typedef struct
{
    int sockfd;
} Request;

// Request queue and associated variables
Request request_queue[QUEUE_SIZE];
int queue_size = 0;
int queue_start = 0;
int queue_end = 0;

// Mutex and condition variables for queue management
pthread_mutex_t queue_mutex;
pthread_cond_t queue_cond;
pthread_cond_t queue_full_cond;

// Function declarations
void *doprocessing(void *sock);
uint64_t bruteForce(unsigned char target_hash[], uint64_t start, uint64_t end);
void *worker_thread(void *arg);
void enqueue_request(int sockfd);
int dequeue_request();

// Structure to hold cached hash data
typedef struct
{
    uint64_t number;
    unsigned char hash[SHA256_DIGEST_LENGTH];
} HashCacheEntry;

HashCacheEntry cache[1000];
int cache_size = 0;
pthread_mutex_t cache_mutex; // Mutex to protect cache access

uint64_t find_cache(unsigned char target_hash[]);
void add_cache(unsigned char target_hash[], uint64_t num);

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Initialize mutexes and condition variables
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
    pthread_cond_init(&queue_full_cond, NULL);
    pthread_mutex_init(&cache_mutex, NULL);

    // Create worker threads
    pthread_t thread_pool[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (pthread_create(&thread_pool[i], NULL, worker_thread, NULL) < 0)
        {
            perror("ERROR creating worker thread");
            exit(1);
        }
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    portno = atoi(argv[1]);
    if (portno <= 0)
    {
        fprintf(stderr, "ERROR, invalid port number\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }

        // Enqueue the new request
        enqueue_request(newsockfd);
    }

    // Destroy the mutex and condition variables before exiting
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
    pthread_cond_destroy(&queue_full_cond);
    pthread_mutex_destroy(&cache_mutex);

    return 0;
}

void enqueue_request(int sockfd)
{
    pthread_mutex_lock(&queue_mutex);
    while (queue_size == QUEUE_SIZE)
    {
        // If the queue is full, wait for a worker to process a request
        pthread_cond_wait(&queue_full_cond, &queue_mutex);
    }

    // Add request to the queue
    request_queue[queue_end].sockfd = sockfd;
    queue_end = (queue_end + 1) % QUEUE_SIZE;
    queue_size++;

    // Signal worker threads that a request is available
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

int dequeue_request()
{
    pthread_mutex_lock(&queue_mutex);
    while (queue_size == 0)
    {
        // If the queue is empty, wait for a request to arrive
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    // Remove request from the queue
    int sockfd = request_queue[queue_start].sockfd;
    queue_start = (queue_start + 1) % QUEUE_SIZE;
    queue_size--;

    // Signal that there is space in the queue
    pthread_cond_signal(&queue_full_cond);
    pthread_mutex_unlock(&queue_mutex);

    return sockfd;
}

void *worker_thread(void *arg)
{
    while (1)
    {
        // Dequeue the next request
        int sockfd = dequeue_request();

        // Process the request
        doprocessing((void *)&sockfd);

        // Close the socket after processing
        close(sockfd);
    }

    return NULL;
}

void *doprocessing(void *sock)
{
    int newsockfd = *((int *)sock);

    int n;
    unsigned char buffer[PACKET_REQUEST_SIZE];
    bzero(buffer, PACKET_REQUEST_SIZE);
    n = read(newsockfd, buffer, PACKET_REQUEST_SIZE);

    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

    uint64_t start, end, result;

    // Extract start and end values
    memcpy(&start, buffer + PACKET_REQUEST_START_OFFSET, sizeof(start));
    start = be64toh(start);

    memcpy(&end, buffer + PACKET_REQUEST_END_OFFSET, sizeof(end));
    end = be64toh(end);

    // Run brute force and write the result back
    result = bruteForce(buffer, start, end);
    uint64_t num_net = htobe64(result);

    n = write(newsockfd, &num_net, sizeof(num_net));
    if (n < 0)
    {
        perror("ERROR writing to socket");
        exit(1);
    }

    return NULL;
}

uint64_t find_cache(unsigned char target_hash[])
{
    pthread_mutex_lock(&cache_mutex);
    for (size_t i = 0; i < cache_size; i++)
    {
        if (memcmp(cache[i].hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
        {
            pthread_mutex_unlock(&cache_mutex);
            return cache[i].number;
        }
    }
    pthread_mutex_unlock(&cache_mutex);
    return 0;
}

void add_cache(unsigned char target_hash[], uint64_t num)
{
    pthread_mutex_lock(&cache_mutex);
    cache[cache_size].number = num;
    memcpy(cache[cache_size].hash, target_hash, SHA256_DIGEST_LENGTH);
    cache_size++;

    pthread_mutex_unlock(&cache_mutex);
}

uint64_t bruteForce(unsigned char target_hash[], uint64_t start, uint64_t end)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    uint64_t a = find_cache(target_hash);
    if (a > 0)
    {
        printf("Match found for number: %llu in cache\n", a);
        return a;
    }

    for (uint64_t num = start; num < end; num++)
    {
        SHA256((unsigned char *)&num, sizeof(num), hash);
        if (memcmp(hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
        {
            printf("Match found for number: %llu\n", num);
            add_cache(hash, num);
            return num;
        }
    }

    printf("No match found.\n");
    return -1;
}
