#ifndef ATM_H
#define ATM_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

typedef struct queue_node {
    int atm_id;
    int priority;
    int is_active;
    char task[64];
    struct queue_node* next;
} queue_node;


typedef struct {
    int atm_id;
    char* filepath;
    int is_active;
} ATMThreadArgs;

typedef struct {
    //int atm_id;
    queue_node* queue_head;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_cond;

} VIP_args;

extern VIP_args* vip_args;
extern int glob_num_atm;
extern int* req_arr;
extern ATMThreadArgs** global_args_arr;

// Add the thread routine prototype
void* atm_thread_routine(void* args);
void process_atm_file(int atm_id, const char* filepath, int is_active);
void trim_newline(char* str);
void process_line(int atm_id, char* line, int is_active);
void add_to_queue(queue_node* new_node);
void* vip_thread_routine(void* args);
void* destroy_vip_threads(void* args);

#endif
