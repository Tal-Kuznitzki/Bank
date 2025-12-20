#ifndef ATM_H
#define ATM_H

#include <stdio.h>
#include <unistd.h>


typedef struct {
    int atm_id;
    char* filepath;
} ATMThreadArgs;

// Add the thread routine prototype
void* atm_thread_routine(void* args);
void process_atm_file(int atm_id, const char* filepath);
void trim_newline(char* str);
void process_line(int atm_id, char* line);

#endif
