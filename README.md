# Virtual Memory Simulation

This project simulates virtual memory with paging, including advanced features such as two-level paging, demand paging, swapping with an LRU algorithm, and Copy-on-Write (COW) during fork operations. The simulation models multiple processes sharing the system, each with their own virtual memory space.

## Features

### Core Features
1. **Two-Level Paging**
   - Simulates hierarchical page tables for efficient memory utilization.
   - First-level and second-level tables dynamically allocated as needed.

2. **Demand Paging**
   - Pages are allocated only when accessed by processes.
   - Handles page faults dynamically by allocating physical frames.

3. **Swapping with LRU**
   - Implements least-recently-used eviction policy when physical memory is full.
   - Evicts pages from memory to a simulated disk store.

4. **Copy-on-Write (COW)**
   - Simulates memory sharing between parent and child processes after a fork.
   - Creates a new memory page only when a write operation occurs.

5. **Detailed Logging**
   - Logs all critical events including:
     - Page faults
     - Frame allocations
     - Swapping in/out
     - Copy-on-Write operations
     - Memory accesses

6. **Simulation**
   - Manages 10 user processes and runs for up to 10,000 ticks.
   - Simulates memory access with read and write operations.

## File Structure

- `virtual_memory_sim.c` - C implementation of the simulation.
- `simulation_logs.txt` - Log file containing details of the simulation.

## How to Build and Run

1. **Compile the Code**
   ```bash
   make
   ```

2. **Run the Simulation**
   ```bash
   ./virtual_memory_sim
   ```

3. **Review the Logs**
   The logs will be saved in `simulation_logs.txt` and include detailed information about memory operations.

## Implementation Details

### Page Table
- Two-level hierarchical structure.
- Each process has a unique first-level page table.
- Second-level tables are dynamically allocated as needed.

### Memory Access
- Virtual addresses are broken into:
  - First-level index
  - Second-level index
  - Page offset
- On access:
  - If the page is valid, translates to a physical address.
  - If invalid, triggers a page fault and allocates a frame.

### Swapping
- When physical memory is full, the LRU policy is applied to evict a page.
- Evicted pages are stored in a simulated disk array.
- Swapped pages are reloaded into memory when accessed.

### Copy-on-Write
- Forked processes share pages with the parent.
- Pages are marked read-only until modified.
- On write, a new physical frame is allocated, and the page is updated.

## Simulation Overview
- Manages 10 user processes.
- Each process accesses 10 random memory addresses per tick.
- Logs all memory operations, including read/write access, page faults, and COW events.
- Terminates after 10,000 ticks.

## Output
- Logs are stored in `simulation_logs.txt`.
- Each log entry includes:
  - Tick number
  - Event type (e.g., page fault, swap out, memory access)
  - Details about the affected process, virtual address, and physical address.

## Example Log Output
```
Tick 1: Process 0 accessed VA 0x1234 -> PA 0x5678 (valid, read)
Tick 2: Process 1 page fault at VA 0x9876.
Tick 3: Swapped out page from frame 12 for Process 2.
Tick 4: Copy-on-Write: Process 3 allocated new frame 45 for VA 0x5678.
```

## Requirements
- **Compiler**: GCC or any C99-compliant compiler.
- **Memory**: Simulated environment requires sufficient RAM for testing purposes.

## Notes
- The simulation is designed for educational purposes and simplifies certain behaviors for clarity.
- Disk size is assumed to be infinite for swapping purposes.
- Performance may vary depending on the system running the simulation.

## Future Enhancements
- Improve swapping policies with additional algorithms (e.g., Clock or FIFO).
- Add multi-threading support for parallel process simulation.
- Extend logging with visualization tools.

---

Developed to demonstrate virtual memory concepts including paging, demand paging, and advanced memory management techniques.

