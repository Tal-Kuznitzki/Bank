#ifndef ATM_H
#define ATM_H

#include <stdio.h>

void process_atm_file(int atm_id, const char* filepath);
void trim_newline(char* str);
void process_line(int atm_id, char* line);

#endif
