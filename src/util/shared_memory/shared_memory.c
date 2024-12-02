//
// Created by hochacha on 11/25/24.
//

#include "shared_memory.h"
SharedMemory* p_mem;
// 공유 메모리 초기화 및 매핑
SharedMemory* init_shared_memory() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }

    // 공유 메모리 크기 설정
    if (ftruncate(shm_fd, SHARED_MEM_SIZE) == -1) {
        perror("ftruncate failed");
        exit(1);
    }

    SharedMemory* shm = (SharedMemory*)mmap(
        NULL,
        SHARED_MEM_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    if (shm == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // 메모리 초기화
    memset(shm->physical_memory, 0, SHARED_MEM_SIZE);

    return shm;
}

// 공유 메모리 정리
void cleanup_shared_memory(SharedMemory* shm) {
    if (munmap(shm, SHARED_MEM_SIZE) == -1) {
        perror("munmap failed");
    }
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink failed");
    }
}