CC = gcc
CFLAGS = -std=c99 -g -Wall -Werror -pedantic-errors -DNDEBUG -pthread
TARGET = bank

all: $(TARGET)

$(TARGET): main.o ATM.o Bank.o
	$(CC) $(CFLAGS) $^ -o $@

main.o: main.c ATM.h Bank.h
	$(CC) $(CFLAGS) -c main.c

ATM.o: ATM.c ATM.h Bank.h
	$(CC) $(CFLAGS) -c ATM.c

Bank.o: Bank.c Bank.h
	$(CC) $(CFLAGS) -c Bank.c

clean:
	rm -f *.o $(TARGET) log.txt