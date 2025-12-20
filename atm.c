#include "atm.h"
#include "bank.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define CMD_LEN 256


// ... existing includes ...

void* atm_thread_routine(void* args) {
    ATMThreadArgs* atm_args = (ATMThreadArgs*)args;

    // Call the logic you already wrote
    process_atm_file(atm_args->atm_id, atm_args->filepath);

    // Free the argument struct allocated in main
    free(atm_args);

    return NULL;
}

// Helper to remove newlines
void trim_newline(char* str) {
    char* p = strchr(str, '\n');
    if (p) *p = 0;
    p = strchr(str, '\r');
    if (p) *p = 0;
}

void process_line(int atm_id, char* line) {
    char logBuffer[512];
    char cmdType;
    int acc_id, password, amount, time_ms, target_id;
    char currency[4]; // ILS or USD
    int init_ils, init_usd;
    
    // Check for VIP (Parsing only, no logic change for single thread)
    // In a real parser, we might strip "VIP=XX" from the end first.
    // For this implementation, we rely on sscanf ignoring trailing format if strict structure isn't enforced,
    // or we manually parse. Given the strict formats, let's assume we parse the command char first.
    
    cmdType = line[0];
    
    // Note: To support "VIP=X" or "PERSISTENT" at the end, standard sscanf might fail 
    // if we don't handle the extra string. For Phase 1, we will parse the main args.
    
    switch (cmdType) {
        case 'O': { // Open: O <acc> <pass> <ils> <usd>
            if (sscanf(line + 2, "%d %d %d %d", &acc_id, &password, &init_ils, &init_usd) == 4) {
                if (find_account(acc_id)) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - account with the same id exists", atm_id);
                } else {
                    add_account(create_account(acc_id, password, init_ils, init_usd));
                    sprintf(logBuffer, "%d: New account id is %d with password %d and initial balance %d ILS and %d USD", 
                        atm_id, acc_id, password, init_ils, init_usd);
                }
                log_msg(logBuffer);
            }
            break;
        }
        case 'D': { // Deposit: D <acc> <pass> <amount> <curr>
            if (sscanf(line + 2, "%d %d %d %s", &acc_id, &password, &amount, currency) == 4) {
                Account* acc = find_account(acc_id);
                if (!acc) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - account id %d does not exist", atm_id, acc_id);
                } else if (acc->password != password) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - password for account id %d is incorrect", atm_id, acc_id);
                } else {
                    if (strcmp(currency, "ILS") == 0) acc->balance_ils += amount;
                    else acc->balance_usd += amount;
                    sprintf(logBuffer, "%d: Account %d new balance is %d ILS and %d USD after %d %s was deposited", 
                        atm_id, acc_id, acc->balance_ils, acc->balance_usd, amount, currency);
                }
                log_msg(logBuffer);
            }
            break;
        }
        case 'W': { // Withdraw: W <acc> <pass> <amount> <curr>
            if (sscanf(line + 2, "%d %d %d %s", &acc_id, &password, &amount, currency) == 4) {
                Account* acc = find_account(acc_id);
                if (!acc) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - account id %d does not exist", atm_id, acc_id);
                } else if (acc->password != password) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - password for account id %d is incorrect", atm_id, acc_id);
                } else {
                    bool enough = (strcmp(currency, "ILS") == 0) ? (acc->balance_ils >= amount) : (acc->balance_usd >= amount);
                    if (!enough) {
                        sprintf(logBuffer, "Error %d: Your transaction failed - account id %d balance is %d ILS and %d USD is lower than %d %s", 
                            atm_id, acc_id, acc->balance_ils, acc->balance_usd, amount, currency);
                    } else {
                        if (strcmp(currency, "ILS") == 0) acc->balance_ils -= amount;
                        else acc->balance_usd -= amount;
                        sprintf(logBuffer, "%d: Account %d new balance is %d ILS and %d USD after %d %s was withdrawn", 
                            atm_id, acc_id, acc->balance_ils, acc->balance_usd, amount, currency);
                    }
                }
                log_msg(logBuffer);
            }
            break;
        }
        case 'B': { // Balance: B <acc> <pass>
            if (sscanf(line + 2, "%d %d", &acc_id, &password) == 2) {
                Account* acc = find_account(acc_id);
                if (!acc) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - account id %d does not exist", atm_id, acc_id);
                } else if (acc->password != password) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - password for account id %d is incorrect", atm_id, acc_id);
                } else {
                    sprintf(logBuffer, "%d: Account %d balance is %d ILS and %d USD", 
                        atm_id, acc_id, acc->balance_ils, acc->balance_usd);
                }
                log_msg(logBuffer);
            }
            break;
        }
        case 'Q': { // Close Account: Q <acc> <pass>
            if (sscanf(line + 2, "%d %d", &acc_id, &password) == 2) {
                Account* acc = find_account(acc_id);
                if (!acc) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - account id %d does not exist", atm_id, acc_id);
                } else if (acc->password != password) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - password for account id %d is incorrect", atm_id, acc_id);
                } else {
                    int bal_ils = acc->balance_ils;
                    int bal_usd = acc->balance_usd;
                    delete_account(acc_id);
                    sprintf(logBuffer, "%d: Account %d is now closed. Balance was %d ILS and %d USD", 
                        atm_id, acc_id, bal_ils, bal_usd);
                }
                log_msg(logBuffer);
            }
            break;
        }
        case 'T': { // Transfer: T <src> <pass> <dst> <amt> <curr>
            if (sscanf(line + 2, "%d %d %d %d %s", &acc_id, &password, &target_id, &amount, currency) == 5) {
                Account* src = find_account(acc_id);
                Account* dst = find_account(target_id);
                
                if (!src) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - account id %d does not exist", atm_id, acc_id);
                } else if (!dst) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - account id %d does not exist", atm_id, target_id);
                } else if (src->password != password) {
                    sprintf(logBuffer, "Error %d: Your transaction failed - password for account id %d is incorrect", atm_id, acc_id);
                } else {
                    bool enough = (strcmp(currency, "ILS") == 0) ? (src->balance_ils >= amount) : (src->balance_usd >= amount);
                    if (!enough) {
                        sprintf(logBuffer, "Error %d: Your transaction failed - account id %d balance is %d ILS and %d USD is lower than %d %s", 
                            atm_id, acc_id, src->balance_ils, src->balance_usd, amount, currency);
                    } else {
                        if (strcmp(currency, "ILS") == 0) {
                            src->balance_ils -= amount;
                            dst->balance_ils += amount;
                        } else {
                            src->balance_usd -= amount;
                            dst->balance_usd += amount;
                        }
                        sprintf(logBuffer, "%d: Transfer %d %s from account %d to account %d new account balance is %d ILS and %d USD new target account balance is %d ILS and %d USD", 
                            atm_id, amount, currency, acc_id, target_id, src->balance_ils, src->balance_usd, dst->balance_ils, dst->balance_usd);
                    }
                }
                log_msg(logBuffer);
            }
            break;
        }
        case 'X': { // Exchange: X <acc> <pass> <curr1> to <curr2> <amt>
            char curr2[4];
            char toWord[4]; // placeholder for "to"
            // X <acc> <pass> <curr1> to <curr2> <amount>
            // Need care with sscanf string parsing
             if (sscanf(line + 2, "%d %d %s %s %s %d", &acc_id, &password, currency, toWord, curr2, &amount) == 6) {
                 Account* acc = find_account(acc_id);
                 if (!acc) {
                     sprintf(logBuffer, "Error %d: Your transaction failed - account id %d does not exist", atm_id, acc_id);
                 } else if (acc->password != password) {
                     sprintf(logBuffer, "Error %d: Your transaction failed - password for account id %d is incorrect", atm_id, acc_id);
                 } else {
                     // Check funds
                     bool isILS = (strcmp(currency, "ILS") == 0);
                     int balance = isILS ? acc->balance_ils : acc->balance_usd;
                     
                     if (balance < amount) {
                        sprintf(logBuffer, "Error %d: Your transaction failed - account id %d balance is %d ILS and %d USD is lower than %d %s",
                             atm_id, acc_id, acc->balance_ils, acc->balance_usd, amount, currency);
                     } else {
                         // Perform exchange: 1 USD = 5 ILS
                         if (isILS) {
                             // ILS to USD
                             acc->balance_ils -= amount;
                             acc->balance_usd += (amount / 5);
                         } else {
                             // USD to ILS
                             acc->balance_usd -= amount;
                             acc->balance_ils += (amount * 5);
                         }
                         sprintf(logBuffer, "%d: Account %d new balance is %d ILS and %d USD after %d %s was exchanged", 
                             atm_id, acc_id, acc->balance_ils, acc->balance_usd, amount, currency);
                     }
                 }
                 log_msg(logBuffer);
             }
             break;
        }
        case 'I': { // Investment: > <acc> <pass> <amount> <curr> <time_ms>
             if (sscanf(line + 2, "%d %d %d %s %d", &acc_id, &password, &amount, currency, &time_ms) == 5) {
                 // Implementation logic:
                 // 1. Find account, check password, check funds.
                 // 2. Deduct funds. 
                 // 3. Since we don't have threads, we cannot "wait" and "return" the money automatically later.
                 //    We will just deduct it and print a message saying it's invested.
                 //    (In a real threaded sim, a thread would sleep and write back).
                 Account* acc = find_account(acc_id);
                 if(acc && acc->password == password) {
                      bool isILS = (strcmp(currency, "ILS") == 0);
                      if ((isILS && acc->balance_ils >= amount) || (!isILS && acc->balance_usd >= amount)) {
                           if(isILS) acc->balance_ils -= amount; else acc->balance_usd -= amount;
                           // Note: No specific log format was given for success in the summary, 
                           // but strict logs are required. I will assume a basic debug log or none if not specified.
                           // The prompt says "do not skip logic".
                           // Let's assume the money disappears for now.
                      double calc = 1;
                      double final_amount = 0;
                      for (int i =0 ; i<(time_ms/10) ; i++) {
                          
                          calc = 1.03*calc;
                      }
                      final_amount = calc*amount;
                      acc->i_start_time = time(NULL);
                      if(isILS) acc->balance_ils += final_amount ; else acc->balance_usd += final_amount;
                      acc->invested_amount = final_amount; 
                 }
                }
                 // Not logging anything because no specific log string was provided in the translation for Investment Success.
             }
             break;
        }
        case 'R': { // Rollback: R <iterations>
            int iterations;
            if (sscanf(line + 1, "%d", &iterations) == 1) {
                if (rollback_bank(iterations) == 0) {
                    sprintf(logBuffer, "%d: Rollback to %d bank iterations ago was completed successfully", atm_id, iterations);
                } else {
                    sprintf(logBuffer, "Error %d: Rollback failed (not enough history)", atm_id); //delete later because it is not defined
                }
                log_msg(logBuffer);
            }
            break;
        }
        case 'S': { // Sleep: S <time>
            // Just parse and log, no actual sleep in single-threaded
            int sleep_time;
            if (sscanf(line + 1, "%d", &sleep_time) == 1) {
                
                sprintf(logBuffer, "%d: Currently on a scheduled break. Service will resume within %d ms.", atm_id, sleep_time);
                log_msg(logBuffer);
                sleep(sleep_time/1000);
            }
            break;
        }
        default:
            break;
    }
}

void process_atm_file(int atm_id, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) {
        printf("Bank error: illegal arguments\n"); // Using spec error format
        exit(1);
    }
    
    char line[CMD_LEN];
    while (fgets(line, CMD_LEN, f)) {
        trim_newline(line);
        if (strlen(line) == 0) continue;
        
        // Take snapshot before every operation to simulate state history
        // In the threaded version, this happens every 500ms. Here, we do it per op to test R logic.
        take_snapshot();

        process_line(atm_id, line);

    }
    fclose(f);
}
