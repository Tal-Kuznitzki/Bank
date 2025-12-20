#include "bank.h"
#include "atm.h"
#include <stdlib.h>

int main(int argc, char* argv[]) {
    // Expected: ./bank <VIP_threads> <ATM1> <ATM2> ...
    if (argc < 3) {
        fprintf(stderr, "Bank error: illegal arguments\n");
        return 1;
    }

    // We ignore VIP_threads count for this single-threaded phase
    // int vip_threads = atoi(argv[1]);

    init_bank();

    // Iterate sequentially through ATM files
    for (int i = 2; i < argc; i++) {
        int atm_id = i - 1; // ATM IDs match the file sequence order (1, 2, 3...)
        process_atm_file(atm_id, argv[i]);
    }

    close_bank();
    return 0;
}
