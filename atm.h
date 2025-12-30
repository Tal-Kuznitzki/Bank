#ifndef ATM_H
#define ATM_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

typedef struct queue_node {
    int atm_id;
    int priority;
    char task[64];
    struct queue_node* next;
} queue_node;


typedef struct {
    int atm_id;
    char* filepath;
} ATMThreadArgs;

typedef struct {
    //int atm_id;
    queue_node* queue_head;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_cond;

} VIP_args;

extern VIP_args* vip_args;

// Add the thread routine prototype
void* atm_thread_routine(void* args);
void process_atm_file(int atm_id, const char* filepath);
void trim_newline(char* str);
void process_line(int atm_id, char* line);
void add_to_queue(queue_node* new_node);
void* vip_thread_routine(void* args);
void* destroy_vip_threads(void* args);

#endif
