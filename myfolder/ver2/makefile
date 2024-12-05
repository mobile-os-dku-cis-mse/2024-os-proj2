CC = gcc
CFLAGS = -Wall -g
LIBS = -lrt

SRCS = sec2.c vmm2.c vmm2_swap.c vmm2_stats.c queue.c
OBJS = $(SRCS:.c=.o)
HEADERS = msg2.h vmm2.h
TARGET = vmm2

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean