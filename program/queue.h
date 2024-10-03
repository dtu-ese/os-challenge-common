#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define QUEUE_SIZE 500

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
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_full_cond = PTHREAD_COND_INITIALIZER;

// Enqueue a request in the request queue
void enqueue_request(int sockfd)
{
    pthread_mutex_lock(&queue_mutex);
    while (queue_size == QUEUE_SIZE)
    {
        pthread_cond_wait(&queue_full_cond, &queue_mutex);
    }

    request_queue[queue_end].sockfd = sockfd;
    queue_end = (queue_end + 1) % QUEUE_SIZE;
    queue_size++;

    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

// Dequeue a request from the request queue
int dequeue_request()
{
    pthread_mutex_lock(&queue_mutex);
    while (queue_size == 0)
    {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    int sockfd = request_queue[queue_start].sockfd;
    queue_start = (queue_start + 1) % QUEUE_SIZE;
    queue_size--;

    pthread_cond_signal(&queue_full_cond);
    pthread_mutex_unlock(&queue_mutex);

    return sockfd;
}

#endif // QUEUE_H
