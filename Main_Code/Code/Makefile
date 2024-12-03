# 컴파일러 설정
CC = gcc
CFLAGS = -Wall -g

# 실행 파일 이름 설정
EXECUTABLES = Basic FIFO LRU Two_level

# 삭제할 추가 파일 설정
CLEANFILES = *.txt

# 기본 타겟: 모든 실행 파일 컴파일
all: $(EXECUTABLES)

# 각 실행 파일에 대한 규칙
Basic: Basic.c
	$(CC) $(CFLAGS) -o Basic Basic.c

FIFO: FIFO.c
	$(CC) $(CFLAGS) -o FIFO FIFO.c

LRU: LRU.c
	$(CC) $(CFLAGS) -o LRU LRU.c

Two_level: Two_level.c
	$(CC) $(CFLAGS) -o Two_level Two_level.c

# 클린업 규칙
clean:
	rm -f $(EXECUTABLES) $(CLEANFILES)

# 다시 빌드 규칙
rebuild: clean all
