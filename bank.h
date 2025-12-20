#ifndef BANK_H
#define BANK_H

#include "account.h"
#include <stdio.h>
#include <pthread.h>

#define MAX_HISTORY 120
#define COMMISSION_INTERVAL 0.03
#define STATUS_PRINT_INTERVAL 0.01

typedef struct BankState {
    Account* account_list;
    // Add global bank funds from commissions if needed
    int bank_ils_profit;
    int bank_usd_profit;
} BankState;

typedef struct Bank {
    BankState current_state;
    
    // History for Rollback
    BankState history[MAX_HISTORY];
    int history_count;
    
    // Logging file
    FILE* log_file;
} Bank;

// Global bank instance (common in these types of OS exercises)
extern Bank g_bank;

void init_bank();
void close_bank();

// Core Logic
Account* find_account(int id);
void add_account(Account* acc);
void delete_account(int id); // Logical close

// Features
void take_snapshot();
int rollback_bank(int iterations); // Returns 0 on success, -1 error
void bank_commission(); // Logic for taking commission
void log_msg(const char* msg);
void print_bank_status();
void check_commission_execution();
#endif
