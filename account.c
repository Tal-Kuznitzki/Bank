#include "account.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Account* create_account(int id, int password, int init_ils, int init_usd) {
    Account* new_acc = (Account*)malloc(sizeof(Account));
    if (!new_acc){
        perror("Bank error: malloc failed");
        return NULL;
    }
    new_acc->id = id;
    new_acc->password = password;
    new_acc->balance_ils = init_ils;
    new_acc->balance_usd = init_usd;
    new_acc->is_active = true;
    new_acc->invested_amount = 0;
    new_acc->investment_duration = 0;
    new_acc->next = NULL;
    new_acc->i_start_time = 0;
    rwlock_init(&new_acc->account_lock);
    return new_acc;
}

Account* copy_account_list(Account* head) {
    if (!head) return NULL;
    Account* new_head = NULL;
    Account* tail = NULL;

    Account* curr = head;
    while (curr) {
        Account* node = (Account*)malloc(sizeof(Account));
        if (node==NULL) {
            perror("Bank error: malloc failed");
            return NULL ;
        }
        // Copy data
        *node = *curr; 
        node->next = NULL;
        rwlock_init(&node->account_lock);

        if (!new_head) {
            new_head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        curr = curr->next;
    }
    return new_head;
}

void free_account_list(Account* head) {
    while (head) {
        Account* temp = head;
        head = head->next;
        rwlock_destroy(&temp->account_lock);
        free(temp);
    }
}
