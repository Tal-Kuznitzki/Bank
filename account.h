#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Account {
    int id;
    int password;
    int balance_ils;
    int balance_usd;
    bool is_active; // For "closing" accounts without freeing immediately if needed for rollback logic context

    // Investment logic
    int invested_amount; // In simulated cents or base units? Spec says int.
    int investment_duration; // ms
    
    struct Account* next; // Linked list pointer
} Account;

// Helper to create a new account
Account* create_account(int id, int password, int init_ils, int init_usd);

// Deep copy for rollback snapshots
Account* copy_account_list(Account* head);

// Frees a list of accounts
void free_account_list(Account* head);

#endif
