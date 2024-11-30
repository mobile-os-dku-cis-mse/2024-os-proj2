#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "logic.h"

int main() {
    srand(time(NULL));
    Kernel *kernel = initialize_kernel();
    initialize_processes(kernel);
    fork_process(kernel, 0);
    simulate(kernel);
    fclose(kernel->log_file);
    return 0;
}
