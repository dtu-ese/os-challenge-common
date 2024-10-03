#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include <stdint.h>
#include <openssl/sha.h>
#include <string.h>
#include <stdio.h>

// Enum for cache status
typedef enum
{
    NONE,
    IN_PROGRESS,
    SOLVED
} CacheStatus;

// Structure for cache entries
typedef struct
{
    uint64_t number;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    CacheStatus status;
    pthread_cond_t condition;
} HashCacheEntry;

// Cache array and cache management variables
HashCacheEntry cache[1000];
int cache_size = 0;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// Find a cache entry with status
int find_cache_with_status(unsigned char target_hash[], uint64_t *result)
{
    pthread_mutex_lock(&cache_mutex);
    for (size_t i = 0; i < cache_size; i++)
    {
        if (memcmp(cache[i].hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
        {
            if (cache[i].status == SOLVED)
            {
                *result = cache[i].number;
                pthread_mutex_unlock(&cache_mutex);
                return SOLVED;
            }
            else if (cache[i].status == IN_PROGRESS)
            {
                pthread_mutex_unlock(&cache_mutex);
                return IN_PROGRESS;
            }
        }
    }
    pthread_mutex_unlock(&cache_mutex);
    return NONE;
}

// Add a cache entry in-progress
void add_cache_in_progress(unsigned char target_hash[])
{
    pthread_mutex_lock(&cache_mutex);
    cache[cache_size].number = 0;
    memcpy(cache[cache_size].hash, target_hash, SHA256_DIGEST_LENGTH);
    cache[cache_size].status = IN_PROGRESS;
    pthread_cond_init(&cache[cache_size].condition, NULL);
    cache_size++;
    pthread_mutex_unlock(&cache_mutex);
}

// Add a solved cache entry
void add_cache(unsigned char target_hash[], uint64_t num)
{
    pthread_mutex_lock(&cache_mutex);
    cache[cache_size].number = num;
    memcpy(cache[cache_size].hash, target_hash, SHA256_DIGEST_LENGTH);
    cache_size++;
    pthread_mutex_unlock(&cache_mutex);
}

#endif // CACHE_H
