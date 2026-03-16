#ifndef CACHE_ARRAY_H
#define CACHE_ARRAY_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Structure for a (hash, key) pair to be stored in the cache
typedef struct Pair {
    uint8_t hash[32];
    uint64_t key;
} pair_t;

// Pair constructor
void init_pair(pair_t* pair, uint8_t *hash, uint64_t key) {
    memcpy(pair->hash, hash, 32);
    pair->key = key;
}

// Structure for a cache of (hash, key) pairs
typedef struct Cache {
    pair_t* arr;
    int size;
    int capacity;
} cache_t;

// Cache constructor
void init_cache(cache_t* cache) {
    cache->arr = malloc(sizeof(pair_t) * 100);
    cache->size = 0;
    cache->capacity = 100;
}

// Add a (hash, key) pair to the cache, resizing if necessary
void cache_insert(cache_t* cache, uint8_t* hash, uint64_t key) {
    // Resize if full
    if (cache->size == cache->capacity) {
        cache->capacity *= 2;
        cache->arr = realloc(cache->arr, sizeof(pair_t) * cache->capacity);
    }
    // Create a pair and save it in the cache
    pair_t pair;
    init_pair(&pair, hash, key);
    cache->arr[cache->size++] = pair;
}

// Search for a (hash, key) pair in the cache given a hash
pair_t *cache_search(cache_t* cache, uint8_t* hash) {
    // Iterate through array
    for (int i = 0; i < cache->size; i++) {
        // If found a match return pointer to the pair
        if (memcmp(cache->arr[i].hash, hash, 32) == 0) {
            return &cache->arr[i];
        }
    }
    // If not in cache, return NULL
    return NULL;
}

#endif //CACHE_ARRAY_H