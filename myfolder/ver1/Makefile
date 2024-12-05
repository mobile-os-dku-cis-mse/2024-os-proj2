CC = gcc
CFLAGS = -Wall
OBJS = queue.o vmm.o sec.o
TARGET = scheduler

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

queue.o: queue.c msg.h
	$(CC) $(CFLAGS) -c queue.c

vmm.o: vmm.c vmm.h msg.h
	$(CC) $(CFLAGS) -c vmm.c

sec.o: sec.c msg.h vmm.h
	$(CC) $(CFLAGS) -c sec.c

clean:
	rm -f $(OBJS) $(TARGET) schedule_dump.txt