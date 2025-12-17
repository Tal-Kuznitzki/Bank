#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Bank.h"
#include "ATM.h"

int main(int argc, char* argv[]) {
    // Usage: ./bank <VIP count> <file1> [file2...]
    if (argc < 3) {
        fprintf(stderr, "Bank error: illegal arguments\n");
        return 1;
    }

    srand(time(NULL)); // Seed for random commissions

    // 1. Init Bank
    Bank centralBank;
    Bank_Init(&centralBank);

    // 2. Init ATM (Use argv[2] as input file for ATM #1)
    ATM atm1;
    ATM_Init(&atm1, 1, argv[2], &centralBank);

    // 3. Run (Sequential for now)
    // In future parallel version: pthread_create here
    ATM_Run(&atm1);

    // In future parallel version: pthread_join here

    // 4. Cleanup
    Bank_Destroy(&centralBank);

    return 0;
}
/**
 * TODO :
 * INPUT PARSER FOR MULTIPLE ATMS
 * INVESTMENT
 * CITE REMOVAL
 *
 */