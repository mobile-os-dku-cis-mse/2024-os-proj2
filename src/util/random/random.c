#include "random.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

distribution_t current_distribution = DISTRIBUTION_UNIFORM;

uint16_t generate_uniform_vaddr() {
    return rand() % 0x10000; // 16비트 주소 범위
}

// Gaussian 분포 파라미터
#define GAUSSIAN_MEAN 32768.0
#define GAUSSIAN_STDDEV 5000.0

uint16_t generate_gaussian_vaddr() {
    // Box-Muller 변환
    double u1 = ((double)rand() + 1) / ((double)RAND_MAX + 2);
    double u2 = ((double)rand() + 1) / ((double)RAND_MAX + 2);
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    
    // 가우시안 분포에 따라 주소 생성
    double vaddr = GAUSSIAN_MEAN + z0 * GAUSSIAN_STDDEV;
    
    // 주소 범위 클램핑
    if (vaddr < 0) vaddr = 0;
    if (vaddr > 0xFFFF) vaddr = 0xFFFF;
    
    return (uint16_t)vaddr;
}

#define MAX_VADDR 0xFFFF
#define ZIPFIAN_S 1.0 // Zipfian 분포 파라미터 (s > 1은 더 많은 skew를 만듭니다)

double zipf_cdf_table[MAX_VADDR + 2];
double HN_zipf = 0.0;

// Zipfian CDF 테이블 초기화
void init_zipf_cdf() {
    zipf_cdf_table[0] = 0.0;
    for(int i = 1; i <= MAX_VADDR + 1; i++) {
        zipf_cdf_table[i] = zipf_cdf_table[i-1] + 1.0 / pow(i, ZIPFIAN_S);
    }
    HN_zipf = zipf_cdf_table[MAX_VADDR + 1];
}

// 이진 탐색을 통한 Zipfian 분포 주소 생성
uint16_t generate_zipfian_vaddr() {
    if(HN_zipf == 0.0) {
        init_zipf_cdf();
    }
    
    double u = ((double)rand()) / RAND_MAX;
    double target = u * HN_zipf;
    
    int low = 1;
    int high = MAX_VADDR + 1;
    int mid;
    while(low < high) {
        mid = (low + high) / 2;
        if(zipf_cdf_table[mid] < target) {
            low = mid + 1;
        }
        else {
            high = mid;
        }
    }
    
    // Rank을 주소로 매핑 (1-based to 0-based)
    return (uint16_t)(low - 1);
}

uint16_t generate_vaddr() {
    switch(current_distribution) {
        case DISTRIBUTION_UNIFORM:
            return generate_uniform_vaddr();
        case DISTRIBUTION_GAUSSIAN:
            return generate_gaussian_vaddr();
        case DISTRIBUTION_ZIPFIAN:
            return generate_zipfian_vaddr();
        default:
            return generate_uniform_vaddr(); // 기본은 Uniform
    }
}