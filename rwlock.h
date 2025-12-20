#ifndef RWLOCK_H
#define RWLOCK_H

#include <pthread.h>

typedef struct {
    pthread_mutex_t write_lock;      // Corresponds to 'wrt_lock' in your image
    pthread_mutex_t read_count_lock; // Corresponds to 'rd_lock' in your image
    int read_count;                  // Corresponds to 'rd_count'
} ReadWriteLock;

// Initialize the lock
void rwlock_init(ReadWriteLock* rw);

// Destroy the lock (clean up mutexes)
void rwlock_destroy(ReadWriteLock* rw);

// Request read access
void read_lock(ReadWriteLock* rw);

// Release read access
void read_unlock(ReadWriteLock* rw);

// Request write access
void write_lock(ReadWriteLock* rw);

// Release write access
void write_unlock(ReadWriteLock* rw);

#endif