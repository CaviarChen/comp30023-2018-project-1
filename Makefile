CC=gcc
CPPFLAGS=-Wall -g -Werror -O3 -std=gnu99


SRC=src/server.o
TARGET=server

all: $(SRC) src/server.o
	$(CC) -o $(TARGET) $(SRC) $(CPPFLAGS) -lm

clean:
	rm -f $(TARGET) src/*.o

