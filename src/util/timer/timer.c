//
// Created by hochacha on 24. 11. 7.
//

#include "timer.h"
#include <stdio.h>
#include <sys/time.h>
#define TIMER_INTERVAL 0
#define TIMER_INTERVAL_USEC 50000

int setup_timer() {
    struct itimerval timer_value;

    timer_value.it_interval.tv_sec = TIMER_INTERVAL;
    timer_value.it_interval.tv_usec = TIMER_INTERVAL_USEC;
    timer_value.it_value.tv_sec = TIMER_INTERVAL;
    timer_value.it_value.tv_usec = TIMER_INTERVAL_USEC;

    if(setitimer(ITIMER_REAL, &timer_value, NULL) == -1) {
        perror("setitimer");
        return -1;
    }
}

int stop_timer() {
    struct itimerval timer_value;

    // 모든 값을 0으로 설정하여 타이머 중지
    timer_value.it_interval.tv_sec = 0;
    timer_value.it_interval.tv_usec = 0;
    timer_value.it_value.tv_sec = 0;
    timer_value.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer_value, NULL) == -1) {
        perror("setitimer");
        return -1;
    }

    return 0;
}