// test_memory.c

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "frame_list.h"

int main() {
    init_physical_memory();

    // 프레임 할당
    int frame_number = allocate_frame();
    if (frame_number == -1) {
        printf("No free frames available.\n");
        return 1;
    }
    printf("Allocated frame number: %d\n", frame_number);

    // 프레임에 데이터 쓰기
    uint8_t *frame_address = &physical_memory[frame_number * PAGE_SIZE];
    const char *data = "Hello, Physical Memory!";
    strncpy((char *)frame_address, data, PAGE_SIZE);

    // 프레임에서 데이터 읽기
    printf("Data in frame %d: %s\n", frame_number, frame_address);

    // 프레임 해제
    free_frame(frame_number);
    printf("Frame %d freed.\n", frame_number);

    return 0;
}