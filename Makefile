CC=gcc
CPPFLAGS=-Wall -g -O3 -std=gnu99 -Wno-unused-result -pthread


SRC=src/server.o src/thread_pool.o src/socket_helper.o
TARGET=server

all: $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CPPFLAGS) -lm

clean:
	rm -f $(TARGET) src/*.o
