//
// Created by hochacha on 11/25/24.
//

#include "page_manager.h"

#include <errno.h>
#include <pthread.h>
#include "../../util/pid_queue/pid_queue.h"
#include "../../util/msg_queue/message_queue.h"
#include "../../util/frame_list/frame_list.h"
#include "../../util/paging/paging.h"
#include "../../util/swap/swap.h"
#include "../../util/tlb/tlb.h"
#include <sys/ipc.h>
#include <sys/msg.h>

extern pcb_t* current_process;
extern u_pt_t* pt_base_register;
extern pthread_mutex_t ptbr_mutex;
extern pcb_queue* ready_queue_schd;
extern pcb_queue* waiting_queue_schd;

tlb_t tlb;

// 초기화 함수
int initialize_page_manager() {
    // 페이지 매니저 스레드 생성
    struct msqid_ds buf;
    if (msgctl(msg_queue_id_page, IPC_STAT, &buf) != -1) {
        printf("Queue current bytes: %lu\n", buf.__msg_cbytes);
        printf("Queue number of messages: %lu\n", buf.msg_qnum);
        printf("Queue max bytes: %lu\n", buf.msg_qbytes);
    }

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_memory_request, NULL) != 0) {
        perror("Failed to create page_manager thread");
        return -1;
    }

    initialize_tlb(&tlb);
    init_msg_queue();


    // 스레드 분리
    pthread_detach(thread_id);
    return 0;
}

// 메모리 접근 요청 처리 스레드
void* handle_memory_request(void* arg) {
    while (1) {
        vaddr_t req_msg = {0,};
        printf("[page manager] msg id = %d\n", msg_queue_id_page);
        // 메시지 큐에서 메모리 접근 요청 수신
        if (msgrcv(msg_queue_id_page, &req_msg, sizeof(vaddr_t) - sizeof(long), 0, IPC_NOWAIT) == -1) {
            if (errno == EINTR) {
                continue; // 시그널 인터럽트 시 계속
            }
            perror("msgrcv in page_manager");
            continue;
        }

        printf("Page Manager: Received memory access request from PID: %d, VADDR: 0x%04X\n", req_msg.pid, req_msg.vaddr);

        // PCB 큐에서 해당 PID의 PCB 찾기
        pcb_t* pcb = find_pcb(ready_queue_schd, req_msg.pid);
        if (pcb == NULL) {
            fprintf(stderr, "Page Manager: PCB not found for PID: %d\n", req_msg.pid);
            // 실패 응답 전송
            access_res_t reply_msg = {0};
            reply_msg.msg_t = MESSAGE_TYPE_VADDR_ACCESS;
            reply_msg.pid = req_msg.pid;
            reply_msg.buf = SWAP_FAIL;
            msgsnd(msg_queue_id_page, &reply_msg, sizeof(access_res_t) - sizeof(long), IPC_NOWAIT);
            continue;
        }

        // 페이지 테이블 접근
        u_pt_t* page_table = pcb->page_table;
        if (page_table == NULL) {
            fprintf(stderr, "Page Manager: Page table is NULL for PID: %d\n", req_msg.pid);
            // 실패 응답 전송
            access_res_t reply_msg = {0};
            reply_msg.msg_t = MESSAGE_TYPE_VADDR_ACCESS;
            reply_msg.pid = req_msg.pid;
            reply_msg.buf = SWAP_FAIL;
            msgsnd(msg_queue_id_page, &reply_msg, sizeof(access_res_t) - sizeof(long), IPC_NOWAIT);
            continue;
        }

        // 주소 변환
        uint32_t paddr;
        int translate_result = translate_address(page_table, &tlb, req_msg.vaddr, &paddr);

        if (translate_result < 0) {
            fprintf(stderr, "Page Manager: Page fault at VADDR: 0x%04X for PID: %d\n", req_msg.vaddr, req_msg.pid);
            // 페이지 폴트 처리 (스왑 인)
            if (swap_in_page(page_table, req_msg.vaddr) != SWAP_SUCCESS) {
                fprintf(stderr, "Page Manager: Failed to swap in page for PID: %d, VADDR: 0x%04X\n", req_msg.pid, req_msg.vaddr);
                // 실패 응답 전송
                access_res_t reply_msg = {0};
                reply_msg.msg_t = MESSAGE_TYPE_VADDR_ACCESS;
                reply_msg.pid = req_msg.pid;
                reply_msg.buf = SWAP_FAIL;
                msgsnd(msg_queue_id_page, &reply_msg, sizeof(access_res_t) - sizeof(long), IPC_NOWAIT);
                continue;
            }

            // 재번역
            translate_result = translate_address(page_table, &tlb, req_msg.vaddr, &paddr);
            if (translate_result < 0) {
                fprintf(stderr, "Page Manager: Failed to translate address after swap in for PID: %d, VADDR: 0x%04X\n", req_msg.pid, req_msg.vaddr);
                // 실패 응답 전송
                access_res_t reply_msg = {0};
                reply_msg.msg_t = MESSAGE_TYPE_VADDR_ACCESS;
                reply_msg.pid = req_msg.pid;
                reply_msg.buf = SWAP_FAIL;
                msgsnd(msg_queue_id_page, &reply_msg, sizeof(access_res_t) - sizeof(long), IPC_NOWAIT);
                continue;
            }
        }

        // 성공 응답 전송
        access_res_t reply_msg = {0};
        reply_msg.msg_t = MESSAGE_TYPE_VADDR_ACCESS;
        reply_msg.pid = req_msg.pid;
        reply_msg.buf = SWAP_SUCCESS;
        msgsnd(msg_queue_id_page, &reply_msg, sizeof(access_res_t) - sizeof(long), IPC_NOWAIT);

        printf("Page Manager: Translated VADDR: 0x%04X to PADDR: 0x%08X for PID: %d\n", req_msg.vaddr, paddr, req_msg.pid);
    }

    return NULL;
}