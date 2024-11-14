#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CACHE_SIZE 1024 // Define a size for the cache

// Structure for a (hash, key) pair to be stored in the cache
typedef struct Pair {
    uint8_t hash[32];
    uint64_t key;
    struct Pair* next; // Pointer for chaining in case of hash collisions
} pair_t;

// Structure for a cache of (hash, key) pairs
typedef struct Cache {
    pair_t** table; // Hash table
    int size;       // Current size of the cache
} cache_t;

// Hash function for the cache
unsigned int hash_function(uint8_t* hash) {
    unsigned int hash_value = 0;
    for (int i = 0; i < 32; i++) {
        hash_value = (hash_value * 31) + hash[i];
    }
    return hash_value % CACHE_SIZE;
}

// Cache constructor
void init_cache(cache_t* cache) {
    cache->table = malloc(sizeof(pair_t*) * CACHE_SIZE);
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache->table[i] = NULL; // Initialize all entries to NULL
    }
    cache->size = 0;
}

// Add a (hash, key) pair to the cache
void cache_insert(cache_t* cache, uint8_t* hash, uint64_t key) {
    unsigned int index = hash_function(hash);
    pair_t* new_pair = malloc(sizeof(pair_t));
    memcpy(new_pair->hash, hash, 32);
    new_pair->key = key;
    new_pair->next = cache->table[index]; // Insert at the head of the list
    cache->table[index] = new_pair;
    cache->size++;
}

// Search for a (hash, key) pair in the cache given a hash
pair_t* cache_search(cache_t* cache, uint8_t* hash) {
    unsigned int index = hash_function(hash);
    pair_t* current = cache->table[index];
    while (current != NULL) {
        if (memcmp(current->hash, hash, 32) == 0) {
            return current; // Found a match
        }
        current = current->next; // Move to the next pair
    }
    return NULL; // Not found
}

// Free the cache
void free_cache(cache_t* cache) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        pair_t* current = cache->table[i];
        while (current != NULL) {
            pair_t* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(cache->table);
}

#endif //CACHE_H