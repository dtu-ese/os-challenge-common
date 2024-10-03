#ifndef PROCESSING_H
#define PROCESSING_H

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "bruteforce.h"
#include "messages.h"
#include <stdlib.h>

// Processing client requests
void *doprocessing(void *sock)
{
    int newsockfd = *((int *)sock);
    unsigned char buffer[PACKET_REQUEST_SIZE];
    bzero(buffer, PACKET_REQUEST_SIZE);

    int n = read(newsockfd, buffer, PACKET_REQUEST_SIZE);
    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

    uint64_t start, end, result;
    // uint8_t prio;
    memcpy(&start, buffer + PACKET_REQUEST_START_OFFSET, sizeof(start));
    start = be64toh(start);
    memcpy(&end, buffer + PACKET_REQUEST_END_OFFSET, sizeof(end));
    end = be64toh(end);
    // memcpy(&prio, buffer + PACKET_REQUEST_PRIO_OFFSET, sizeof(prio));
    // prio = be64toh(prio);

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

#endif // PROCESSING_H
