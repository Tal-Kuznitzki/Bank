#define _DEFAULT_SOURCE // For usleep
#include "ATM.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void ATM_Init(ATM* atm, int id, const char* inputFile, Bank* bank) {
    atm->id = id;
    atm->inputFile = inputFile;
    atm->bankPtr = bank;
}

void ATM_Run(ATM* atm) {
    FILE* fp = fopen(atm->inputFile, "r");
    if (!fp) {
        [cite_start]// PDF Requirement: Error to stderr [cite: 268]
        fprintf(stderr, "Bank error: illegal arguments\n");
        exit(1);
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {

        // Simulation delay (100ms per PDF cycle structure)
        usleep(100000);

        char cmd[10];
        if (sscanf(line, "%s", cmd) < 1) continue;
        char c = cmd[0];

        // O: Open
        if (c == 'O') {
            int accId, pass, balILS, balUSD;
            sscanf(line, "%*s %d %d %d %d", &accId, &pass, &balILS, &balUSD);
            Bank_OpenAccount(atm->bankPtr, accId, pass, balILS, balUSD, atm->id);
        }
            // D: Deposit
        else if (c == 'D') {
            int accId, pass, amount; char curr[4];
            sscanf(line, "%*s %d %d %d %s", &accId, &pass, &amount, curr);
            Bank_Deposit(atm->bankPtr, accId, pass, amount, curr, atm->id);
        }
            // W: Withdraw
        else if (c == 'W') {
            int accId, pass, amount; char curr[4];
            sscanf(line, "%*s %d %d %d %s", &accId, &pass, &amount, curr);
            Bank_Withdraw(atm->bankPtr, accId, pass, amount, curr, atm->id);
        }
            // B: Balance
        else if (c == 'B') {
            int accId, pass;
            sscanf(line, "%*s %d %d", &accId, &pass);
            Bank_GetBalance(atm->bankPtr, accId, pass, atm->id);
        }
            // Q: Close Account
        else if (c == 'Q') {
            int accId, pass;
            sscanf(line, "%*s %d %d", &accId, &pass);
            Bank_CloseAccount(atm->bankPtr, accId, pass, atm->id);
        }
            // T: Transfer
        else if (c == 'T') {
            int srcId, pass, dstId, amount; char curr[4];
            sscanf(line, "%*s %d %d %d %d %s", &srcId, &pass, &dstId, &amount, curr);
            Bank_Transfer(atm->bankPtr, srcId, pass, dstId, amount, curr, atm->id);
        }
            // X: Exchange
        else if (c == 'X') {
            int id, pass, amount; char srcCurr[4], dstCurr[4];
            // "X <id> <pass> <src> to <dst> <amount>"
            // Note: sscanf %*s skips the "to"
            sscanf(line, "%*s %d %d %s %*s %s %d", &id, &pass, srcCurr, dstCurr, &amount);
            Bank_Exchange(atm->bankPtr, id, pass, srcCurr, dstCurr, amount, atm->id);
        }
            // R: Rollback
        else if (c == 'R') {
            int iter;
            // "R <iterations>" or "R<iterations>" depending on space.
            // If space: "R 2" -> sscanf OK. If "R2" -> cmd is "R2".
            [cite_start]// PDF example: "R1"[cite: 221].
            if (strlen(cmd) > 1) {
                // Parse number from rest of cmd string
                iter = atoi(cmd + 1);
            } else {
                sscanf(line, "%*s %d", &iter);
            }
            Bank_Rollback(atm->bankPtr, iter, atm->id);
        }
            // C: Close ATM
        else if (c == 'C') {
            int targetId;
            // Example: "C1"
            if (strlen(cmd) > 1) {
                targetId = atoi(cmd + 1);
            } else {
                sscanf(line, "%*s %d", &targetId);
            }
            Bank_CloseATM(atm->bankPtr, targetId, atm->id);
        }
            // >: Invest
        else if (c == '>') {
            // Example parsing: "> 12345 1234 100 ILS 5"
            int id, pass, amount, time; char curr[4];
            sscanf(line, "%*s %d %d %d %s %d", &id, &pass, &amount, curr, &time);
            Bank_Invest(atm->bankPtr, id, pass, amount, curr, time, atm->id);
        }

        // Simulation: Action time (1 sec)
        // sleep(1);
    }
    fclose(fp);
}