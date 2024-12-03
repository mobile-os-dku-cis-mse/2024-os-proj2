#pragma once

#include "common.h"

void initialize_timer(void (*handler)(int));
void start_timer();
void stop_timer();
