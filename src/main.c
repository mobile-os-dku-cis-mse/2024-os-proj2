#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process_manager.h"

int main() {
    // 설정 파일 경로
    const char *config_file = "config.txt";

    // 설정값을 저장할 변수들
    int time_tick = 0;
    int page_replace = 0;
    int tlb_cache = 0;
    int page_size = 0;
    int memory_size = 0;
    int tlb_size = 0;
    int status = 0;
    int random = 0;

    // 설정 파일 열기
    FILE *file = fopen(config_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open configuration file: %s\n", config_file);
        return EXIT_FAILURE;
    }

    // 설정 파일에서 값 읽기
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "time_tick_limit=%d", &time_tick) == 1) continue;
        if (sscanf(line, "page_replacement_algorithm=%d", &page_replace) == 1) continue;
        if (sscanf(line, "tlb_cache_replacement=%d", &tlb_cache) == 1) continue;
        if (sscanf(line, "page_size_kb=%d", &page_size) == 1) continue;
        if (sscanf(line, "physical_memory_size_mb=%d", &memory_size) == 1) continue;
        if (sscanf(line, "tlb_cache_size=%d", &tlb_size) == 1) continue;
        if (sscanf(line, "status=%d", &status) == 1) continue;
        if (sscanf(line, "random=%d", &random) == 1) continue;
    }

    // 파일 닫기
    fclose(file);

    // 읽은 설정값 출력 (디버깅용)
    printf("\n========= CONFIG SUMMARY =========\n");
    printf("Configuration Loaded:\n\n");
    printf("  Time Tick Limit: %d\n", time_tick);
    printf("  Page Replacement Algorithm: %d\n", page_replace);
    printf("  TLB Cache Replacement: %d\n", tlb_cache);
    printf("  Page Size (KB): %d\n", page_size);
    printf("  Physical Memory Size (MB): %d\n", memory_size);
    printf("  TLB Cache Size: %d\n", tlb_size);
    printf("  Status: %d\n", status);
    printf("  Random: %d\n", random);
    printf("==================================\n\n");

    // Process Manager 초기화
    srand((unsigned)time(NULL));
    char *tar_name = "example.tar"; // tar 파일 경로

    // init_process_manager 호출
    process_manager(tar_name, time_tick, page_replace, tlb_cache, page_size, memory_size, tlb_size, status, random);

    return 0;
}
