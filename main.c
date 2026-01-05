#define _POSIX_C_SOURCE 199309L
#include "bank.h"
#include "atm.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "rwlock.h"

//queue_node* queue_head = NULL;
VIP_args* vip_args = NULL;
int glob_num_atm = 0;
int* req_arr = NULL;
ATMThreadArgs** global_args_arr;
ReadWriteLock req_arr_lock;
int num_VIP_threads;

void* status_printer_thread(void* arg) {
    struct timespec ts;
    ts.tv_sec = STATUS_PRINT_INTERVAL /1000 ;
    ts.tv_nsec = ( (STATUS_PRINT_INTERVAL % 1000)*1000*1000) ;
    int requests_remain=0;
    int target_id = -1;
    int source_id = -1;

    read_lock(&req_arr_lock);
    for (int i = 0; i <glob_num_atm ; ++i) {

        requests_remain +=req_arr[i];
    }
    read_unlock(&req_arr_lock);
    while (system_running || requests_remain ) {
        requests_remain=0;
        print_bank_status();
        read_lock(&req_arr_lock);
        for (int i = 0; i <glob_num_atm ; ++i) {
            requests_remain +=req_arr[i];
        }
        read_unlock(&req_arr_lock);
        for (int i = 0 ; i<glob_num_atm ; i++){
            if (req_arr[i] != 0){
                write_lock(&(global_args_arr[i]->active_lock));
                write_lock(&req_arr_lock);
                read_unlock(&req_arr_lock);
                global_args_arr[i]->is_active = 0;
                target_id = i + 1;
                source_id = req_arr[i];
                char msg[256];
                sprintf(msg, "Bank: ATM %d closed %d successfully", source_id, target_id);
                log_msg(msg);
                
                //read_unlock(&req_arr_lock);

                req_arr[i]=0;
                write_unlock(&req_arr_lock);
                write_unlock(&(global_args_arr[i]->active_lock));
            }
        }
        // Sleep for 10ms
        nanosleep(&ts,NULL);
    }

    return NULL;
}

void* commission_thread(void* arg) {
    struct timespec ts;
    ts.tv_sec = COMMISSION_INTERVAL /1000 ;
    ts.tv_nsec = ( (COMMISSION_INTERVAL % 1000)*1000*1000) ;

    while (system_running) {
        bank_commission();
        nanosleep(&ts,NULL);
    }
    return NULL;
}
void* snapshot_thread(void* arg) {
    struct timespec ts;
    ts.tv_sec = SNAPSHOT_INTERVAL_TIME /1000 ;
    ts.tv_nsec = ( (SNAPSHOT_INTERVAL_TIME % 1000)*1000*1000) ;

    while (system_running) {
        take_snapshot();
        nanosleep(&ts,NULL);
    }
    return NULL;
}



int main(int argc, char* argv[]) {
    // Expected: ./bank <VIP_threads> <ATM1> <ATM2> ...
    if (argc < 3) {
        fprintf(stderr, "Bank error: illegal arguments\n");
        return 1;
    }
    int num_atms = argc - 2;
     if  ( num_atms<= 0 ){
        //TODO ERROR!!
        return -1;
    }
    glob_num_atm = num_atms;
    req_arr = malloc(sizeof(int)*num_atms);
    if (req_arr == NULL) {
        perror("Bank error: malloc failed");
        exit(1);
    }

    init_bank();

    rwlock_init(&req_arr_lock);

    pthread_t* atm_threads = malloc(sizeof(pthread_t) * num_atms);
    if(atm_threads==NULL){
        perror("Bank error: malloc failed");
        return 1 ;
    }
    pthread_t* vip_threads = NULL;
    num_VIP_threads=atoi(argv[1]);
      if (num_VIP_threads>0){
        vip_threads = malloc(sizeof(pthread_t) * num_VIP_threads);
        if(vip_threads==NULL){
            perror("Bank error: malloc failed");
            return 1 ;
        }

        vip_args = malloc(sizeof(VIP_args));
          if(vip_args==NULL){
              perror("Bank error: malloc failed");
              return 1 ;
          }

        vip_args->queue_head = NULL;
        pthread_mutex_init(&vip_args->queue_lock, NULL);
        pthread_cond_init(&vip_args->queue_cond, NULL);
        for (int i = 0; i < num_VIP_threads ; ++i) {
           
            if (pthread_create(&vip_threads[i], NULL, vip_thread_routine, vip_args) != 0) {
                perror("Bank error: pthread_create failed");
                return 1;
            }
        }

    }
    //#threads = #files + 2 (one for printing times) (one for investment)
    pthread_t printer_tid;

  

    if (pthread_create(&printer_tid, NULL, status_printer_thread, NULL) != 0) {
        perror("Bank error: pthread_create failed");
        return 1;
    }
    pthread_t commission_tid;
    if (pthread_create(&commission_tid, NULL, commission_thread, NULL) != 0) {
        perror("Bank error: pthread_create failed");
        return 1;
    }
    pthread_t snapshot_tid;
    if (pthread_create(&snapshot_tid, NULL, snapshot_thread, NULL) != 0) {
        perror("Bank error: pthread_create failed");
        return 1;
    }


    global_args_arr = malloc(sizeof(ATMThreadArgs*) * num_atms);
    if(global_args_arr==NULL){
        perror("Bank error: malloc failed");
        return 1 ;
    }

    // Iterate sequentially through ATM files
    for (int i = 0; i < num_atms; i++) {
        req_arr[i]=0;
        ATMThreadArgs* args = malloc(sizeof(ATMThreadArgs));
        if(args==NULL){
            perror("Bank error: malloc failed");
            return 1 ;
        }
        args->atm_id = i + 1; // ATMs are 1-based usually
        args->filepath = argv[i + 2]; // Skip ./bank and VIP_count
        args->is_active = 1;
        rwlock_init(&args->active_lock);
        global_args_arr[i]=args;  

        if (pthread_create(&atm_threads[i], NULL, atm_thread_routine, args) != 0) {
            perror("Bank error: pthread_create failed");
            free(atm_threads);
            return 1;
        }
    }

    // 3. Wait for all ATMs to finish
    // -------------------------------------------------
    for (int i = 0; i < num_atms; i++) {
        pthread_join(atm_threads[i], NULL);
    }



    system_running=0;
/*    pthread_cancel(printer_tid);
    pthread_cancel(commission_tid);
    pthread_cancel(snapshot_tid);*/
    if (num_VIP_threads>0){
        pthread_mutex_lock(&vip_args->queue_lock);
        pthread_cond_broadcast(&vip_args->queue_cond);
        pthread_mutex_unlock(&vip_args->queue_lock);

        for (int i = 0; i < num_VIP_threads; i++) {
            pthread_join(vip_threads[i], NULL);
        }

        pthread_mutex_lock(&vip_args->queue_lock);
        queue_node* curr = vip_args->queue_head;
        while (curr != NULL) {
            queue_node* temp = curr;
            curr = curr->next;
            free(temp); // Delete the memory of the remaining task
        }
        //vip_args->queue_head = NULL;
        pthread_mutex_unlock(&vip_args->queue_lock);

        // Clean up resources ONLY after all threads have joined
        pthread_mutex_destroy(&vip_args->queue_lock);
        pthread_cond_destroy(&vip_args->queue_cond);
        free(vip_args);
        free(vip_threads);
    }

    for (int i = 0; i < num_atms; i++) {
        if (global_args_arr[i] != NULL) {
            rwlock_destroy(&(global_args_arr[i]->active_lock));
            free(global_args_arr[i]); 
        }
    }

    free(global_args_arr);

    pthread_join(printer_tid, NULL);
    pthread_join(commission_tid, NULL);
    pthread_join(snapshot_tid, NULL);
    rwlock_destroy(&req_arr_lock);
    free(atm_threads);
    close_bank();
    return 0;
}
