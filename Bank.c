#include "Bank.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For sleep if needed

// --- Internal Helpers ---

static Account* findAccount(Bank* bank, int id) {
    Account* current = bank->accounts_head;
    while (current != NULL) {
        if (current->id == id) return current;
        current = current->next;
    }
    return NULL;
}

// Deep Copy: Live List -> Snapshot Data
static AccountData* copyAccountListToData(Account* head) {
    if (!head) return NULL;
    AccountData* newHead = NULL;
    AccountData* tail = NULL;
    Account* current = head;

    while (current) {
        AccountData* newNode = (AccountData*)malloc(sizeof(AccountData));
        newNode->id = current->id;
        newNode->password = current->password;
        newNode->balanceILS = current->balanceILS;
        newNode->balanceUSD = current->balanceUSD;
        newNode->is_active = current->is_active;
        newNode->next = NULL;

        if (!newHead) {
            newHead = newNode;
            tail = newHead;
        } else {
            tail->next = newNode;
            tail = tail->next;
        }
        current = current->next;
    }
    return newHead;
}

// Deep Copy: Snapshot Data -> Live List (Restoring)
static Account* copyDataToAccountList(AccountData* head) {
    if (!head) return NULL;
    Account* newHead = NULL;
    Account* tail = NULL;
    AccountData* current = head;

    while (current) {
        Account* newNode = (Account*)malloc(sizeof(Account));
        newNode->id = current->id;
        newNode->password = current->password;
        newNode->balanceILS = current->balanceILS;
        newNode->balanceUSD = current->balanceUSD;
        newNode->is_active = current->is_active;
        newNode->next = NULL;

        if (!newHead) {
            newHead = newNode;
            tail = newHead;
        } else {
            tail->next = newNode;
            tail = tail->next;
        }
        current = current->next;
    }
    return newHead;
}

static void freeSnapshot(Snapshot* snap) {
    if (!snap) return;
    AccountData* current = snap->head;
    while (current) {
        AccountData* temp = current;
        current = current->next;
        free(temp);
    }
    free(snap);
}

// --- Implementation ---

void Bank_Init(Bank* bank) {
    bank->accounts_head = NULL;
    bank->logFile = fopen("log.txt", "w");
    bank->bank_balance_ILS = 0;
    bank->bank_balance_USD = 0;
    bank->history_count = 0;

    for (int i = 0; i < MAX_HISTORY; i++) {
        bank->BankBalanceCommisionILS[i] = 0;
        bank->BankBalanceCommisionUSD[i] = 0;
        bank->Backup[i] = NULL;
    }
}

void Bank_Destroy(Bank* bank) {
    if (bank->logFile) fclose(bank->logFile);

    // Free live accounts
    Account* current = bank->accounts_head;
    while (current) {
        Account* temp = current;
        current = current->next;
        free(temp);
    }

    // Free history
    for (int i = 0; i < MAX_HISTORY; i++) {
        if (bank->Backup[i]) freeSnapshot(bank->Backup[i]);
    }
}

[cite_start]// [cite: 230-234]
void Bank_PrintStatus(Bank* bank) {
    printf("\033[2J");      // Clear screen
    printf("\033[1;1H");    // Reset cursor
    printf("Current Bank Status\n");

    Account* current = bank->accounts_head;
    while (current) {
        if (current->is_active) {
            printf("Account %d: Balance %d ILS %d USD, Account Password - %d\n",
                   current->id,
                   current->balanceILS,
                   current->balanceUSD,
                   current->password);
        }
        current = current->next;
    }
}

void Bank_MakeSnapshot(Bank* bank) {
    int idx = bank->history_count % MAX_HISTORY;

    // Clear old backup if it exists
    if (bank->Backup[idx]) freeSnapshot(bank->Backup[idx]);

    Snapshot* snap = (Snapshot*)malloc(sizeof(Snapshot));
    if (snap) {
        snap->head = copyAccountListToData(bank->accounts_head);
        bank->Backup[idx] = snap;
    }

    bank->BankBalanceCommisionILS[idx] = bank->bank_balance_ILS;
    bank->BankBalanceCommisionUSD[idx] = bank->bank_balance_USD;
    bank->history_count++;
}

int Bank_Rollback(Bank* bank, int iterations, int atmID) {
    if (iterations > bank->history_count || iterations > MAX_HISTORY) return -1;

    // Calculate index to revert to
    // Logic: If current count is 5, R1 means we want state 4.
    // Abs index = count - iterations.
    int targetAbs = bank->history_count - iterations;
    int targetIdx = targetAbs % MAX_HISTORY;

    Snapshot* snap = bank->Backup[targetIdx];
    if (!snap) return -1; // Safety check

    // 1. Destroy current live state
    Account* current = bank->accounts_head;
    while (current) {
        Account* temp = current;
        current = current->next;
        free(temp);
    }

    // 2. Restore from backup
    bank->accounts_head = copyDataToAccountList(snap->head);
    bank->bank_balance_ILS = bank->BankBalanceCommisionILS[targetIdx];
    bank->bank_balance_USD = bank->BankBalanceCommisionUSD[targetIdx];

    // Note: We do not decrement history_count. Rollback is a new event in time.

    fprintf(bank->logFile, "%d: Rollback to %d bank iterations ago was completed successfully\n", atmID, iterations);
    return 0;
}

[cite_start]// [cite: 70-71, 226-229]
void Bank_CollectCommission(Bank* bank) {
    // Generate random percentage 1-5%
    // Note: rand() is not thread safe by default, usually need rand_r for pthreads
    int percentage = (rand() % 5) + 1;

    Account* current = bank->accounts_head;
    while (current) {
        if (current->is_active) {
            [cite_start]// Using integer math as permitted [cite: 261]
            int commILS = (int)((double)current->balanceILS * ((double)percentage / 100.0));
            int commUSD = (int)((double)current->balanceUSD * ((double)percentage / 100.0));

            current->balanceILS -= commILS;
            current->balanceUSD -= commUSD;

            bank->bank_balance_ILS += commILS;
            bank->bank_balance_USD += commUSD;

            fprintf(bank->logFile, "Bank: commissions of %d %% were charged, bank gained %d ILS and %d USD from account %d\n",
                    percentage, commILS, commUSD, current->id);
        }
        current = current->next;
    }
}

// --- Transactions ---

void Bank_OpenAccount(Bank* bank, int id, int pass, int initILS, int initUSD, int atmID) {
    // Check duplicates
    Account* current = bank->accounts_head;
    while (current) {
        if (current->id == id) {
            fprintf(bank->logFile, "Error %d: Your transaction failed - account with the same id exists\n", atmID);
            return;
        }
        current = current->next;
    }

    [cite_start]// Insert Sorted [cite: 231]
    Account* newAcc = (Account*)malloc(sizeof(Account));
    newAcc->id = id;
    newAcc->password = pass;
    newAcc->balanceILS = initILS;
    newAcc->balanceUSD = initUSD;
    newAcc->is_active = true;
    newAcc->next = NULL;

    if (bank->accounts_head == NULL || bank->accounts_head->id > id) {
        newAcc->next = bank->accounts_head;
        bank->accounts_head = newAcc;
    } else {
        Account* temp = bank->accounts_head;
        while (temp->next != NULL && temp->next->id < id) {
            temp = temp->next;
        }
        newAcc->next = temp->next;
        temp->next = newAcc;
    }

    fprintf(bank->logFile, "%d: New account id is %d with password %d and initial balance %d ILS and %d USD\n",
            atmID, id, pass, initILS, initUSD);
}

void Bank_Deposit(Bank* bank, int id, int pass, int amount, const char* currency, int atmID) {
    Account* acc = findAccount(bank, id);
    if (!acc || !acc->is_active) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d does not exist\n", atmID, id);
        return;
    }
    if (acc->password != pass) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - password for account id %d is incorrect\n", atmID, id);
        return;
    }

    if (strcmp(currency, "ILS") == 0) acc->balanceILS += amount;
    else acc->balanceUSD += amount;

    fprintf(bank->logFile, "%d: Account %d new balance is %d ILS and %d USD after %d %s was deposited\n",
            atmID, id, acc->balanceILS, acc->balanceUSD, amount, currency);
}

void Bank_Withdraw(Bank* bank, int id, int pass, int amount, const char* currency, int atmID) {
    Account* acc = findAccount(bank, id);
    if (!acc || !acc->is_active) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d does not exist\n", atmID, id);
        return;
    }
    if (acc->password != pass) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - password for account id %d is incorrect\n", atmID, id);
        return;
    }

    int* balance = (strcmp(currency, "ILS") == 0) ? &acc->balanceILS : &acc->balanceUSD;
    if (*balance < amount) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d balance is %d ILS and %d USD is lower than %d %s\n",
                atmID, id, acc->balanceILS, acc->balanceUSD, amount, currency);
        return;
    }

    *balance -= amount;
    fprintf(bank->logFile, "%d: Account %d new balance is %d ILS and %d USD after %d %s was withdrawn\n",
            atmID, id, acc->balanceILS, acc->balanceUSD, amount, currency);
}

void Bank_GetBalance(Bank* bank, int id, int pass, int atmID) {
    Account* acc = findAccount(bank, id);
    if (!acc || !acc->is_active) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d does not exist\n", atmID, id);
        return;
    }
    if (acc->password != pass) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - password for account id %d is incorrect\n", atmID, id);
        return;
    }

    fprintf(bank->logFile, "%d: Account %d balance is %d ILS and %d USD\n",
            atmID, id, acc->balanceILS, acc->balanceUSD);
}

void Bank_CloseAccount(Bank* bank, int id, int pass, int atmID) {
    Account* acc = findAccount(bank, id);
    if (!acc || !acc->is_active) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d does not exist\n", atmID, id);
        return;
    }
    if (acc->password != pass) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - password for account id %d is incorrect\n", atmID, id);
        return;
    }

    int balILS = acc->balanceILS;
    int balUSD = acc->balanceUSD;
    acc->is_active = false; // "Closed"

    fprintf(bank->logFile, "%d: Account %d is now closed. Balance was %d ILS and %d USD\n",
            atmID, id, balILS, balUSD);
}

[cite_start]// [cite: 125, 164-165]
void Bank_Transfer(Bank* bank, int srcId, int pass, int dstId, int amount, const char* currency, int atmID) {
    // 1. Check Source Existence
    Account* src = findAccount(bank, srcId);
    if (!src || !src->is_active) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d does not exist\n", atmID, srcId);
        return;
    }
    // 2. Check Target Existence
    Account* dst = findAccount(bank, dstId);
    if (!dst || !dst->is_active) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d does not exist\n", atmID, dstId);
        return;
    }
    // 3. Check Password
    if (src->password != pass) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - password for account id %d is incorrect\n", atmID, srcId);
        return;
    }
    // 4. Check Balance
    int* srcBal = (strcmp(currency, "ILS") == 0) ? &src->balanceILS : &src->balanceUSD;
    if (*srcBal < amount) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d balance is %d ILS and %d USD is lower than %d %s\n",
                atmID, srcId, src->balanceILS, src->balanceUSD, amount, currency);
        return;
    }

    // Execute
    *srcBal -= amount;
    if (strcmp(currency, "ILS") == 0) dst->balanceILS += amount;
    else dst->balanceUSD += amount;

    fprintf(bank->logFile, "%d: Transfer %d %s from account %d to account %d new account balance is %d ILS and %d USD new target account balance is %d ILS and %d USD\n",
            atmID, amount, currency, srcId, dstId, src->balanceILS, src->balanceUSD, dst->balanceILS, dst->balanceUSD);
}

[cite_start]// [cite: 89, 167]
void Bank_Exchange(Bank* bank, int id, int pass, const char* srcCurr, const char* dstCurr, int amount, int atmID) {
    Account* acc = findAccount(bank, id);
    if (!acc || !acc->is_active) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d does not exist\n", atmID, id);
        return;
    }
    if (acc->password != pass) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - password for account id %d is incorrect\n", atmID, id);
        return;
    }

    int* srcBal = (strcmp(srcCurr, "ILS") == 0) ? &acc->balanceILS : &acc->balanceUSD;
    int* dstBal = (strcmp(dstCurr, "ILS") == 0) ? &acc->balanceILS : &acc->balanceUSD;

    if (*srcBal < amount) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d balance is %d ILS and %d USD is lower than %d %s\n",
                atmID, id, acc->balanceILS, acc->balanceUSD, amount, srcCurr);
        return;
    }

    *srcBal -= amount;
    int converted = 0;
    if (strcmp(srcCurr, "USD") == 0 && strcmp(dstCurr, "ILS") == 0) converted = amount * 5;
    else if (strcmp(srcCurr, "ILS") == 0 && strcmp(dstCurr, "USD") == 0) converted = amount / 5;
    else converted = amount; // Should not happen per spec

    *dstBal += converted;

    fprintf(bank->logFile, "%d: Account %d new balance is %d ILS and %d USD after %d %s was exchanged\n",
            atmID, id, acc->balanceILS, acc->balanceUSD, amount, srcCurr);
}

void Bank_Invest(Bank* bank, int id, int pass, int amount, const char* currency, int time, int atmID) {
    Account* acc = findAccount(bank, id);
    if (!acc || !acc->is_active) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d does not exist\n", atmID, id);
        return;
    }
    if (acc->password != pass) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - password for account id %d is incorrect\n", atmID, id);
        return;
    }

    int* bal = (strcmp(currency, "ILS") == 0) ? &acc->balanceILS : &acc->balanceUSD;
    if (*bal < amount) {
        fprintf(bank->logFile, "Error %d: Your transaction failed - account id %d balance is %d ILS and %d USD is lower than %d %s\n",
                atmID, id, acc->balanceILS, acc->balanceUSD, amount, currency);
        return;
    }

    *bal -= amount;

    fprintf(bank->logFile, "%d: Account %d new balance is %d ILS and %d USD after %d %s was invested for %d sec\n",
            atmID, id, acc->balanceILS, acc->balanceUSD, amount, currency, time);
}

void Bank_CloseATM(Bank* bank, int targetAtmID, int currentAtmID) {
    // In sequential mode, this is just a log entry
    fprintf(bank->logFile, "Bank: ATM %d closed %d successfully\n", currentAtmID, targetAtmID);
}