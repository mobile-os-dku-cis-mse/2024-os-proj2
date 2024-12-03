//
// Created by hochacha on 11/25/24.
//
#include <sys/msg.h>
#include "page_manager.h"
#include "../util/msg_queue/message_queue.h"

/*
    System Architecture

    Virttual Memory
    - 16bit address space
      - 4bit upper index
      - 4bit lower index
      - 8bit offset
    - 256B page size
    - 256 page entries

    Physical Memory
    - 32bit address space
    - 16KB sized shared memory
    - 256B page size

    Swapping
    - 1MB File (let the file stores 4096 pages)
    - 256B page size

*/

void page_manager(){
    while(1){

        // spinning check
        vaddr_t vaddr_msg = {0,};
        if (msgrcv(msg_queue_id_page, &vaddr_msg, sizeof(vaddr_msg), 0, IPC_NOWAIT) != -1) {
            /*
             *  Vitual Memory Address Translation
             */


            if (msgsnd(msg_queue_id_page, &vaddr_msg, sizeof(vaddr_msg), IPC_NOWAIT) != -1) {
                /*
                 *  Send memory access result
                 */
            }
        }
    }
}