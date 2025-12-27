#define _POSIX_C_SOURCE 199309L
#include "bank.h"
#include "atm.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
void* status_printer_thread(void* arg) {
    struct timespec ts;
    ts.tv_sec = STATUS_PRINT_INTERVAL /1000 ;
    ts.tv_nsec = ( (STATUS_PRINT_INTERVAL % 1000)*1000*1000) ;

    while (system_running) {
        print_bank_status();
        // Sleep for 10ms
        nanosleep(&ts,NULL);
    }
    return NULL;
}
void* commission_thread(void* arg) {
    while(system_running) {
        bank_commission(); // You implemented this in bank.c
        double  commission_slp_tme =  COMMISSION_INTERVAL/1000 ;
       sleep(commission_slp_tme); // 30ms
    }

    return NULL;
}
void* snapshot_thread(void* arg) {
    while(system_running) {
        take_snapshot();
        double  snpsht_slp_tme =  STATUS_PRINT_INTERVAL/1000 ;
        sleep(snpsht_slp_tme); // 30ms
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
        printf("errorrrrr");
        return -1;
    }

    //rwlock_init()
    pthread_t* atm_threads = malloc(sizeof(pthread_t) * num_atms);
   // int num_VIP_threads=atoi(argv[1]);
    // We ignore VIP_threads count for this single-threaded phase
    // int vip_threads = atoi(argv[1]);
    init_bank();
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




    // Iterate sequentially through ATM files
    for (int i = 0; i < num_atms; i++) {
        ATMThreadArgs* args = malloc(sizeof(ATMThreadArgs));
        args->atm_id = i + 1; // ATMs are 1-based usually
        args->filepath = argv[i + 2]; // Skip ./bank and VIP_count

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

    pthread_join(printer_tid, NULL);
    pthread_join(commission_tid, NULL);
    pthread_join(snapshot_tid, NULL);
    free(atm_threads);
    close_bank();
    return 0;
}
