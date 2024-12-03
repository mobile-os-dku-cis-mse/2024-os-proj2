CC = gcc

CFLAGS = -O2 -Wall -g

TARGET = virtual

SRC = main.c log.c virtual_memory.c process.c queue.c scheduling.c timer.c replacement_policy.c

OBJ = $(SRC:.c=.o)

HEADERS = common.h log.h virtual_memory.h process.h queue.h scheduling.h timer.h replacement_policy.h

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
