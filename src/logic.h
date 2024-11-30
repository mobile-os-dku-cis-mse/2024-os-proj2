#ifndef LOGIC_H
#define LOGIC_H

#include <stdbool.h>
#include "struct.h"

Kernel* initialize_kernel();
void initialize_processes(Kernel *kernel);
void fork_process(Kernel *kernel, int parent_pid);
void swap_out(Kernel *kernel);
void handle_page_fault(Kernel *kernel, Process *process, int first_index, int second_index);
void access_page(Kernel *kernel, Process *process, int virtual_address, bool is_write);
void simulate(Kernel *kernel);

#endif //LOGIC_H
