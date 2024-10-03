CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lssl -lcrypto
TARGET = x86_64/bin/linux/server
SRC = program/server.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

