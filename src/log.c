#include <stdio.h>
#include "log.h"

void log_message(Kernel *kernel, const char *message) {
    fprintf(kernel->log_file, "Tick %d: %s\n", kernel->current_tick, message);
}