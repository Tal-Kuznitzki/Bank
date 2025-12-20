#include "bank.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

Bank g_bank;

void init_bank() {
    g_bank.current_state.account_list = NULL;
    g_bank.current_state.bank_ils_profit = 0;
    g_bank.current_state.bank_usd_profit = 0;
    g_bank.history_count = 0;
    g_bank.log_file = fopen("log.txt", "w");
}

void close_bank() {
    free_account_list(g_bank.current_state.account_list);
    for (int i = 0; i < g_bank.history_count; i++) {
        free_account_list(g_bank.history[i].account_list);
    }
    if (g_bank.log_file) fclose(g_bank.log_file);
}

void log_msg(const char* msg) {
    if (g_bank.log_file) {
        fprintf(g_bank.log_file, "%s\n", msg);
        fflush(g_bank.log_file);
    }
    // Also print to stdout for debugging this phase
    printf("%s\n", msg);
}

Account* find_account(int id) {
    Account* curr = g_bank.current_state.account_list;
    while (curr) {
        if (curr->id == id && curr->is_active) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void add_account(Account* acc) {
    // Add to head
    acc->next = g_bank.current_state.account_list;
    g_bank.current_state.account_list = acc;
}

void delete_account(int id) {
    // We don't remove nodes to keep history simple? 
    // Actually, for rollback to work with pointers, deep copies are used in snapshots.
    // Here we can just mark is_active = false or remove it.
    // Let's remove for cleaner list, but careful with deep copy logic.
    Account* curr = g_bank.current_state.account_list;
    Account* prev = NULL;
    while (curr) {
        if (curr->id == id) {
            if (prev) prev->next = curr->next;
            else g_bank.current_state.account_list = curr->next;
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void take_snapshot() {
    // Shift history if full? The spec says max 100-120. 
    // We will just fill up to MAX. Real implementation might need circular buffer.
    if (g_bank.history_count >= MAX_HISTORY) {
        // Remove oldest? Or stop saving? 
        // For this exercise, usually we shift.
        free_account_list(g_bank.history[0].account_list);
        for(int i=0; i < MAX_HISTORY - 1; i++) {
            g_bank.history[i] = g_bank.history[i+1];
        }
        g_bank.history_count--;
    }
    
    int idx = g_bank.history_count;
    g_bank.history[idx].account_list = copy_account_list(g_bank.current_state.account_list);
    g_bank.history[idx].bank_ils_profit = g_bank.current_state.bank_ils_profit;
    g_bank.history[idx].bank_usd_profit = g_bank.current_state.bank_usd_profit;
    g_bank.history_count++;
}

int rollback_bank(int iterations) {
    if (g_bank.history_count < iterations) return -1;
    
    // Target index
    int target_idx = g_bank.history_count - iterations;
    
    // Free current
    free_account_list(g_bank.current_state.account_list);
    
    // Restore state (Deep copy from history, so history remains valid)
    g_bank.current_state.account_list = copy_account_list(g_bank.history[target_idx].account_list);
    g_bank.current_state.bank_ils_profit = g_bank.history[target_idx].bank_ils_profit;
    g_bank.current_state.bank_usd_profit = g_bank.history[target_idx].bank_usd_profit;
    
    // In a real rollback, do we delete the "future" history? 
    // Usually yes.
    for (int i = target_idx + 1; i < g_bank.history_count; i++) {
        free_account_list(g_bank.history[i].account_list);
    }
    g_bank.history_count = target_idx; // Reset history pointer
    
    return 0;
}

void bank_commission() {
    // Logic: take 1-5% from all accounts
    int pct = (rand() % 5) + 1; // 1 to 5
    double factor = (double)pct / 100.0;
    
    Account* curr = g_bank.current_state.account_list;
    while(curr) {
        if(curr->is_active) {
            int comm_ils = (int)(curr->balance_ils * factor);
            int comm_usd = (int)(curr->balance_usd * factor);
            
            curr->balance_ils -= comm_ils;
            curr->balance_usd -= comm_usd;
            
            g_bank.current_state.bank_ils_profit += comm_ils;
            g_bank.current_state.bank_usd_profit += comm_usd;
            
            char buf[256];
            sprintf(buf, "Bank: commissions of %d %% were charged, bank gained %d ILS and %d USD from account %d", 
                pct, comm_ils, comm_usd, curr->id);
            log_msg(buf);
        }
        curr = curr->next;
    }
}
