#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"
#include "thread_pool.h"
#include "cache.h"
#include "bruteforce.h"
#include "processing.h"

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Initialize mutexes and condition variables
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
    pthread_cond_init(&queue_full_cond, NULL);
    pthread_mutex_init(&cache_mutex, NULL);

    // Create and initialize worker threads
    initialize_thread_pool();

    // Set up the server socket
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

    // Main server loop to accept connections
    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }

        // Enqueue the new request for processing
        enqueue_request(newsockfd);
    }

    // Clean up before exit (though we will likely never get here)
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
    pthread_cond_destroy(&queue_full_cond);
    pthread_mutex_destroy(&cache_mutex);

    return 0;
}
