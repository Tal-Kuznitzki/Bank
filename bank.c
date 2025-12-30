#include "bank.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>



volatile int system_running = 1;

Bank g_bank;


// ... existing code ...

// This function corresponds to the "Status Print Thread"
void print_bank_status() {
    // 1. Move cursor to top-left and clear screen [cite: 250]
    // Note: In single-threaded debug, this might wipe your history.


   printf("\033[2J");
   printf("\033[1;1H");

    printf("Current Bank Status\n");
    read_lock(&g_bank.list_lock);
    Account* curr = g_bank.current_state.account_list;
    while (curr !=NULL) {
        if (curr->is_active) {
            // [cite: 243] Format: Account <id>: Balance <ILS> ILS <USD> USD, Account Password - <pass>
            printf("Account %d: Balance %d ILS %d USD, Account Password - %04d\n",
                   curr->id,
                   curr->balance_ils,
                   curr->balance_usd,
                   curr->password);
        }
        curr = curr->next;
    }
    read_unlock(&g_bank.list_lock);

}

// This function corresponds to the "Commission Thread" logic
// Call this function to simulate checking if a commission is due
void init_bank() {
    system_running=1;
    g_bank.current_state.account_list = NULL;
    g_bank.current_state.bank_ils_profit = 0;
    g_bank.current_state.bank_usd_profit = 0;
    g_bank.history_count = 0;
    g_bank.log_file = fopen("log.txt", "w");


    rwlock_init(&g_bank.list_lock);
    rwlock_init(&g_bank.history_list_lock);



    if (pthread_mutex_init(&g_bank.profits_lock, NULL) != 0) {
        perror("Bank error: mutex init failed");
        exit(1);
    }
    if (pthread_mutex_init(&g_bank.log_lock, NULL) != 0) {
        perror("Bank error: mutex init failed");
        exit(1);
    }

}

void close_bank() {

    write_lock(&g_bank.list_lock);
    free_account_list(g_bank.current_state.account_list);
    write_unlock(&g_bank.list_lock);

    for (int i = 0; i < g_bank.history_count; i++) {
        write_lock(&g_bank.history_list_lock);
        free_account_list(g_bank.history[i].account_list);
        write_unlock(&g_bank.history_list_lock);
    }
    if (g_bank.log_file) fclose(g_bank.log_file);
    //loglock destroy
    pthread_mutex_destroy(&g_bank.log_lock);
    pthread_mutex_destroy(&g_bank.profits_lock);
    rwlock_destroy(&g_bank.list_lock);
    rwlock_destroy(&g_bank.history_list_lock);
    }

void log_msg(const char* msg) {
    //lock - there can be only one writing
    pthread_mutex_lock(&g_bank.log_lock);
    if (g_bank.log_file) {
        fprintf(g_bank.log_file, "%s\n", msg);
        fflush(g_bank.log_file);
    }
    // Also print to stdout for debugging this phase
 //   printf("%s\n", msg);

    // unlock - allow the rest to log
    pthread_mutex_unlock(&g_bank.log_lock);
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
    if (g_bank.current_state.account_list == NULL || g_bank.current_state.account_list->id > acc->id) {
        acc->next = g_bank.current_state.account_list;
        g_bank.current_state.account_list = acc;
    } else {
        Account* temp =  g_bank.current_state.account_list ;
        while (temp->next != NULL && temp->next->id < acc->id) {
            temp = temp->next;
        }
        acc->next = temp->next;
        temp->next = acc;
    }


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
            rwlock_destroy(&curr->account_lock);
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void take_snapshot() {
    write_lock(&g_bank.history_list_lock);
    // 1. If Full: Free Oldest, then Shift Left
    if (g_bank.history_count == MAX_HISTORY) {
        // [IMPORTANT] Free the memory of the oldest snapshot before overwriting
        free_account_list(g_bank.history[0].account_list);

        // Shift: [1]->[0], [2]->[1], ...
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            g_bank.history[i] = g_bank.history[i + 1];
        }

        // Decrement count because we effectively removed one
        g_bank.history_count--;
    }

    // 2. Insert New at the end
    int idx = g_bank.history_count; // This is the next free slot

    read_lock(&g_bank.list_lock);
    // Deep copy current live state into history
    g_bank.history[idx].account_list = copy_account_list(g_bank.current_state.account_list);

    pthread_mutex_lock(&g_bank.profits_lock);
    g_bank.history[idx].bank_ils_profit = g_bank.current_state.bank_ils_profit;
    g_bank.history[idx].bank_usd_profit = g_bank.current_state.bank_usd_profit;
    pthread_mutex_unlock(&g_bank.profits_lock);

    read_unlock(&g_bank.list_lock);

    g_bank.history_count++;
    write_unlock(&g_bank.history_list_lock);
}

int rollback_bank(int iterations) {
//
    char logBuffer[512]; //DEBUG
    sprintf(logBuffer,"OPENED LOCKS !!!!!!!!!!!!!!!");
    write_lock(&g_bank.history_list_lock);

    // 1. Validation
    if (iterations > g_bank.history_count || iterations <= 0) {
        write_unlock(&g_bank.history_list_lock);
        return -1;
    }

    write_lock(&g_bank.list_lock);

    // 2. Math: Find the target index
    // If count is 5 (0..4), and we rollback 1, we want index 4.
    int target_idx = g_bank.history_count - iterations;

    // 3. Clear the Live state (it is being abandoned)
    free_account_list(g_bank.current_state.account_list);

    // 4. Restore target state to Live (Deep Copy)
    g_bank.current_state.account_list = copy_account_list(g_bank.history[target_idx].account_list);

    pthread_mutex_lock(&g_bank.profits_lock);
    g_bank.current_state.bank_ils_profit = g_bank.history[target_idx].bank_ils_profit;
    g_bank.current_state.bank_usd_profit = g_bank.history[target_idx].bank_usd_profit;
    pthread_mutex_unlock(&g_bank.profits_lock);

    // 5. [CRITICAL] Wipe the "Future"
    // We must free the history slots we just jumped over (including the one we just restored from,
    // because it is now Live and we don't want a duplicate pointer confusion later).
    for (int i = target_idx; i < g_bank.history_count; i++) {
        free_account_list(g_bank.history[i].account_list);
    }

    // 6. Reset Time
    g_bank.history_count = target_idx;

    write_unlock(&g_bank.list_lock);
    write_unlock(&g_bank.history_list_lock);
    log_msg(logBuffer);
    return 0;

}
void bank_commission() {
    // Logic: take 1-5% from all accounts
    int pct = (rand() % 5) + 1; // 1 to 5
    double factor = (double)pct / 100.0;
    read_lock(&g_bank.list_lock);
    Account* curr = g_bank.current_state.account_list;
    while(curr != NULL ) {
        if(curr->is_active) {
            write_lock(&curr->account_lock);
            int comm_ils = (int)(curr->balance_ils * factor);
            int comm_usd = (int)(curr->balance_usd * factor);

            curr->balance_ils -= comm_ils;
            curr->balance_usd -= comm_usd;

            pthread_mutex_lock(&g_bank.profits_lock);
            g_bank.current_state.bank_ils_profit += comm_ils;
            g_bank.current_state.bank_usd_profit += comm_usd;
            pthread_mutex_unlock(&g_bank.profits_lock);
            
            char buf[256];
            sprintf(buf, "Bank: commissions of %d %% were charged, bank gained %d ILS and %d USD from account %d", 
                pct, comm_ils, comm_usd, curr->id);
            log_msg(buf);
            write_unlock(&curr->account_lock);
        }
        curr = curr->next;
    }
    read_unlock(&g_bank.list_lock);
}
