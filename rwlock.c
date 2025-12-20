#include "rwlock.h"

void rwlock_init(ReadWriteLock* rw) {
    pthread_mutex_init(&rw->write_lock, NULL);
    pthread_mutex_init(&rw->read_count_lock, NULL);
    rw->read_count = 0;
}

void rwlock_destroy(ReadWriteLock* rw) {
    pthread_mutex_destroy(&rw->write_lock);
    pthread_mutex_destroy(&rw->read_count_lock);
}

// READER ENTRY
void read_lock(ReadWriteLock* rw) {
    // 1. Lock generic "read_lock" mutex (protects the counter)
    pthread_mutex_lock(&rw->read_count_lock);

    // 2. Increment reader count
    rw->read_count++;

    // 3. If I am the FIRST reader, I must lock out writers
    if (rw->read_count == 1) {
        pthread_mutex_lock(&rw->write_lock);
    }

    // 4. Release counter lock so other readers can enter
    pthread_mutex_unlock(&rw->read_count_lock);
}

// READER EXIT
void read_unlock(ReadWriteLock* rw) {
    // 1. Lock generic "read_lock" mutex (protects the counter)
    pthread_mutex_lock(&rw->read_count_lock);

    // 2. Decrement reader count
    rw->read_count--;

    // 3. If I am the LAST reader leaving, I must unlock writers
    if (rw->read_count == 0) {
        pthread_mutex_unlock(&rw->write_lock);
    }

    // 4. Release counter lock
    pthread_mutex_unlock(&rw->read_count_lock);
}

// WRITER ENTRY
void write_lock(ReadWriteLock* rw) {
    pthread_mutex_lock(&rw->write_lock);
}

// WRITER EXIT
void write_unlock(ReadWriteLock* rw) {
    pthread_mutex_unlock(&rw->write_lock);
}