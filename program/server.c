#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <openssl/sha.h>

#define PORT 5003 // 8080
#define BUFFER_SIZE 1024

// Structure to hold client request data
typedef struct {
    unsigned char target_hash[SHA256_DIGEST_LENGTH];
    uint64_t start;
    uint64_t end;
    int client_socket;
} request_t;

// Function to compute the SHA256 of a 64-bit number
void compute_sha256(uint64_t num, unsigned char output_hash[SHA256_DIGEST_LENGTH]) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, &num, sizeof(uint64_t));
    SHA256_Final(output_hash, &sha256);
}

// Function to compare two SHA256 hashes
int compare_hashes(unsigned char hash1[SHA256_DIGEST_LENGTH], unsigned char hash2[SHA256_DIGEST_LENGTH]) {
    return memcmp(hash1, hash2, SHA256_DIGEST_LENGTH) == 0;
}

// Brute-force search function to find the original number that produced the hash
void* reverse_hash(void* arg) {
    request_t* request = (request_t*)arg;
    unsigned char computed_hash[SHA256_DIGEST_LENGTH];

    for (uint64_t i = request->start; i < request->end; i++) {
        compute_sha256(i, computed_hash);
        if (compare_hashes(computed_hash, request->target_hash)) {
            // Found the matching number, send it back to the client
            write(request->client_socket, &i, sizeof(uint64_t));
            close(request->client_socket);
            free(request);
            return NULL;
        }
    }

    // No match found, send a failure response
    uint64_t not_found = 0xFFFFFFFFFFFFFFFF;
    write(request->client_socket, &not_found, sizeof(uint64_t));
    close(request->client_socket);
    free(request);
    return NULL;
}

// Function to handle incoming client connections
void handle_client(int client_socket) {
    unsigned char buffer[BUFFER_SIZE];
    ssize_t bytes_received = read(client_socket, buffer, BUFFER_SIZE);

    if (bytes_received < 48) {  // 32 bytes for the hash, 8 bytes for start, 8 bytes for end
        close(client_socket);
        return;
    }

    // Parse the request data
    request_t* request = (request_t*)malloc(sizeof(request_t));
    memcpy(request->target_hash, buffer, SHA256_DIGEST_LENGTH);
    request->start = *(uint64_t*)(buffer + SHA256_DIGEST_LENGTH);
    request->end = *(uint64_t*)(buffer + SHA256_DIGEST_LENGTH + sizeof(uint64_t));
    request->client_socket = client_socket;

    // Create a new thread to handle the brute-force search
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, reverse_hash, request);
    pthread_detach(thread_id);  // Automatically reclaim thread resources after completion
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    // Accept incoming connections and handle them
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }
        handle_client(client_socket);
    }

    return 0;
}
