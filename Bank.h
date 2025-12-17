#ifndef BANK_H
#define BANK_H

#include <stdio.h>
#include <stdbool.h>

#define MAX_HISTORY 120

// --- Account Structures ---

// Account Node for the live Linked List
typedef struct Account {
    int id;
    int password;
    int balanceILS;
    int balanceUSD;
    bool is_active;
    int investedAmount;

    // Future Parallelism: Add pthread_mutex_t lock; or pthread_rwlock_t lock; here
    // to protect individual account access (e.g., Transfer needs to lock two accounts).

    struct Account* next;
} Account;

// Snapshot Node (Deep copy for backup)
typedef struct AccountData {
    int id;
    int password;
    int balanceILS;
    int balanceUSD;
    bool is_active;
    int investedAmount;
    struct AccountData* next;
} AccountData;

// Snapshot Container (Represents one point in time)
typedef struct Snapshot {
    AccountData* head;
} Snapshot;

// --- Bank Structure ---

typedef struct Bank {
    Account* accounts_head;

    // Future Parallelism: Add pthread_rwlock_t list_lock; here
    // to protect the linked list structure (insertions/deletions).

    FILE* logFile;
    // Future Parallelism: Add pthread_mutex_t log_lock; here

    // Bank's private funds
    int bank_balance_ILS;
    int bank_balance_USD;

    // --- History / Backup Arrays ---
    int BankBalanceCommisionILS[MAX_HISTORY];
    int BankBalanceCommisionUSD[MAX_HISTORY];
    Snapshot* Backup[MAX_HISTORY];

    int history_count; // Total snapshots taken (monotonically increasing)
} Bank;

// --- Lifecycle Functions ---
void Bank_Init(Bank* bank);
void Bank_Destroy(Bank* bank);

// --- Visualization & Maintenance ---
void Bank_PrintStatus(Bank* bank);
void Bank_MakeSnapshot(Bank* bank);
void Bank_CollectCommission(Bank* bank); // Logic for taking 1-5%

// --- ATM Transaction Functions ---
// All return void as they handle their own logging internally per spec.
void Bank_OpenAccount(Bank* bank, int id, int pass, int initILS, int initUSD, int atmID);
void Bank_Deposit(Bank* bank, int id, int pass, int amount, const char* currency, int atmID);
void Bank_Withdraw(Bank* bank, int id, int pass, int amount, const char* currency, int atmID);
void Bank_GetBalance(Bank* bank, int id, int pass, int atmID);
void Bank_CloseAccount(Bank* bank, int id, int pass, int atmID);
void Bank_Transfer(Bank* bank, int srcId, int pass, int dstId, int amount, const char* currency, int atmID);
void Bank_Exchange(Bank* bank, int id, int pass, const char* srcCurr, const char* dstCurr, int amount, int atmID);
void Bank_Invest(Bank* bank, int id, int pass, int amount, const char* currency, int time, int atmID);

// --- System Functions ---
void Bank_CloseATM(Bank* bank, int targetAtmID, int currentAtmID);
int  Bank_Rollback(Bank* bank, int iterations, int atmID);

#endif