#include <string.h>
#ifndef REQUEST_H
#define REQUEST_H

// Object-oriented request structure for use within priority queue
typedef struct request {
    uint8_t hash[32];
    uint64_t start;
    uint64_t end;
    uint8_t p;
    int newsockfd;
    int order;
} request_t;

// Constructor
void init_request(request_t* req, uint8_t* hash, uint64_t start, uint64_t end, uint8_t p, int newsockfd, int order) {
    memcpy(req->hash, hash, 32);
    req->start = start;
    req->end = end;
    req->p = p;
    req->newsockfd = newsockfd;
    req->order = order;
}

// Comparator for requests based first on priority, then on order received
int compare_requests(request_t* req1, request_t* req2) {
    if (req1->p == req2->p)
        return req2->order - req1->order;
    return req1->p - req2->p;
}

#endif //REQUEST_H