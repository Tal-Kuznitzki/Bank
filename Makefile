CC = gcc
CFLAGS = -std=c99 -g -Wall -Werror -pedantic-errors
LDFLAGS =

OBJS = main.o bank.o account.o atm.o

bank: $(OBJS)
	$(CC) $(CFLAGS) -o bank $(OBJS) $(LDFLAGS)

main.o: main.c bank.h atm.h
	$(CC) $(CFLAGS) -c main.c

bank.o: bank.c bank.h account.h
	$(CC) $(CFLAGS) -c bank.c

account.o: account.c account.h
	$(CC) $(CFLAGS) -c account.c

atm.o: atm.c atm.h bank.h
	$(CC) $(CFLAGS) -c atm.c

clean:
	rm -f bank *.o log.txt
