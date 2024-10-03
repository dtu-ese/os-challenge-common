#ifndef BRUTEFORCE_H
#define BRUTEFORCE_H

#include <stdint.h>
#include <openssl/sha.h>
#include "cache.h"
#include <stdio.h>

// Brute force function
uint64_t bruteForce(unsigned char target_hash[], uint64_t start, uint64_t end)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    uint64_t result;

    int status = find_cache_with_status(target_hash, &result);
    if (status == SOLVED)
    {
        printf("Match found in cache for number: %llu\n", result);
        return result;
    }
    else if (status == IN_PROGRESS)
    {
        printf("Waiting for another thread to finish processing the same hash...\n");
        pthread_mutex_lock(&cache_mutex);
        while (status == IN_PROGRESS)
        {
            for (size_t i = 0; i < cache_size; i++)
            {
                if (memcmp(cache[i].hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
                {
                    pthread_cond_wait(&cache[i].condition, &cache_mutex);
                    result = cache[i].number;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&cache_mutex);
        return result;
    }

    add_cache_in_progress(target_hash);

    for (uint64_t num = start; num < end; num++)
    {
        SHA256((unsigned char *)&num, sizeof(num), hash);
        if (memcmp(hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
        {
            printf("Match found for number: %llu\n", num);
            pthread_mutex_lock(&cache_mutex);
            for (size_t i = 0; i < cache_size; i++)
            {
                if (memcmp(cache[i].hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
                {
                    cache[i].number = num;
                    cache[i].status = SOLVED;
                    pthread_cond_broadcast(&cache[i].condition);
                    break;
                }
            }
            pthread_mutex_unlock(&cache_mutex);
            return num;
        }
    }

    printf("No match found.\n");
    pthread_mutex_lock(&cache_mutex);
    for (size_t i = 0; i < cache_size; i++)
    {
        if (memcmp(cache[i].hash, target_hash, SHA256_DIGEST_LENGTH) == 0)
        {
            cache[i].status = NONE;
            pthread_cond_broadcast(&cache[i].condition);
            break;
        }
    }
    pthread_mutex_unlock(&cache_mutex);

    return -1;
}

#endif // BRUTEFORCE_H
