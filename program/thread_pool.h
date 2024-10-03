#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "queue.h"
#include "processing.h"

#define THREAD_POOL_SIZE 4

pthread_t thread_pool[THREAD_POOL_SIZE];

// Worker thread function
void *worker_thread(void *arg)
{
    while (1)
    {
        int sockfd = dequeue_request();
        doprocessing((void *)&sockfd);
        close(sockfd);
    }
    return NULL;
}

// Initialize the thread pool
void initialize_thread_pool()
{
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (pthread_create(&thread_pool[i], NULL, worker_thread, NULL) < 0)
        {
            perror("ERROR creating worker thread");
            exit(1);
        }
    }
}

#endif // THREAD_POOL_H
