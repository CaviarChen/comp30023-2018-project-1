CC=gcc
CPPFLAGS=-Wall -g -O3 -std=gnu99 -Wno-unused-result


SRC=src/server.o
TARGET=server

all: $(SRC) src/thread_pool.o src/server.o
	$(CC) -o $(TARGET) $(SRC) $(CPPFLAGS) -lm

clean:
	rm -f $(TARGET) src/*.o
