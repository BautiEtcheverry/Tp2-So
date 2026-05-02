CC     = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -I../Kernel/include -I.

SOURCES = \
    AllTests.c \
    CuTest.c \
    test_memory_stub.c \
    processes/testScheduler.c \
    ../Kernel/processes/process.c \
    ../Kernel/processes/scheduler.c

TARGET = tests

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
