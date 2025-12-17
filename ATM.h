#ifndef ATM_H
#define ATM_H

#include "Bank.h"

typedef struct ATM {
    int id;
    const char* inputFile;
    Bank* bankPtr;
    // Future Parallelism: Add pthread_t thread_id; here
} ATM;

// Initialize
void ATM_Init(ATM* atm, int id, const char* inputFile, Bank* bank);

// Run Loop (Parses file and calls Bank functions)
void ATM_Run(ATM* atm);

#endif