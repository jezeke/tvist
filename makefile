CC = gcc
CFLAGS = -Wall -lpthread -g
TARGET = tvist

all: $(TARGET)

$(TARGET) : tvist.c tvist.h
	$(CC) $(CFLAGS) -o $(TARGET) tvist.c

clean :
	rm tvist
