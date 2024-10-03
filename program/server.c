#include <stdio.h>
#include <stdlib.h>
// #include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include "messages.h"
#include <string.h>

void doprocessing(int sock);
uint64_t bruteForce(unsigned char target_hash[], uint64_t start, uint64_t end);

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int pid;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = 5002;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Bind the host address using bind() call */
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Start listening for clients */
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

        /* Create child process */
        pid = fork();
        if (pid < 0)
        {
            perror("ERROR on fork");
            exit(1);
        }

        if (pid == 0)
        {
            /* This is the client process */
            close(sockfd);
            doprocessing(newsockfd);
            exit(0);
        }
        else
        {
            close(newsockfd);
        }
    }
}
void doprocessing(int sock)
{
    int n;
    unsigned char buffer[49]; // 32 (hash) + 8 (start) + 8 (end) + 1 (p)
    bzero(buffer, 49);
    n = read(sock, buffer, 49); // Read the full request packet

    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

    // Print the received hash in hexadecimal
    printf("Received hash:");
    for (size_t i = 0; i < 32; i++)
    {
        printf("%02x ", buffer[i]); // Print each byte of the hash
    }
    printf("\n");

    // Extract and print the 'start' value (next 8 bytes)
    uint64_t start;
    memcpy(&start, buffer + 32, sizeof(start)); // Copy from buffer into start
    start = be64toh(start);                     // Convert from big-endian to host byte order
    printf("Start: %llu\n", (unsigned long long)start);

    // Extract and print the 'end' value (next 8 bytes)
    uint64_t end;
    memcpy(&end, buffer + 32 + 8, sizeof(end)); // Copy from buffer into end
    end = be64toh(end);                         // Convert from big-endian to host byte order
    printf("End: %llu\n", (unsigned long long)end);

        // Brute-force the number based on the received hash
    uint64_t result = bruteForce(buffer, start, end);

    uint64_t num_net = htobe64(result); // Convert to network byte order

    // Send the result back to the client
    n = write(sock, &num_net, sizeof(num_net));
    if (n < 0)
    {
        perror("ERROR writing to socket");
        exit(1);
    }
}

// Function to brute-force SHA256 hash and return the matching number
uint64_t bruteForce(unsigned char target_hash[], uint64_t start, uint64_t end)
{
    uint64_t num = 0;
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // Try brute-forcing numbers from 0 upwards
    for (num = start; num < end; num++)
    // You can increase the range as needed
    { // Hash the number

        SHA256((unsigned char *)&num, sizeof(num), hash);

        // Compare the generated hash with the target hash
        if (memcmp(hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
        {
            printf("Match found for number: %llu\n", num);
            return num;
        }
    }

    // If no match is found, return -1
    printf("No match found.\n");
    return -1;
}