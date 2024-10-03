
    uint8_t prio;
    memcpy(&prio, buffer + 32 + 8, sizeof(prio)); // Copy from buffer into prio
    prio = be64toh(prio);                         // Convert from big-prioian to host byte order
    printf("prio: %llu\n", (unsigned long long)prio);