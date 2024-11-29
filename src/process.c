#include "OS_memory.h"
#include "communication.h"
#include <time.h>

void kernel_process(int msg_queue_id, int *free_frames, int total_frames)
{
    int free_frame_count = total_frames;
    PageTable page_tables[NUM_PROCESSES];
    for (int i = 0; i < NUM_PROCESSES; i++) {
        initialize_page_table(&page_tables[i]);
    }

    IPCMessage msg;
    for (size_t tick = 0; tick < MAX_TICKS; tick++) {
        // Recevoir une requête utilisateur
        msgrcv(msg_queue_id, &msg, sizeof(IPCMessage) - sizeof(long), 1, 0);

        // Traiter la requête
        int pa = translate_address(free_frames, &free_frame_count, &page_tables[msg.process_id], msg.virtual_address);

        // Répondre avec l'adresse physique
        msg.physical_address = pa;
        msg.page_fault = !page_tables[msg.process_id].entries[msg.virtual_address / PAGE_SIZE].valid;
        msg.msg_type = msg.process_id + 2; // Retourner au processus spécifique
        msgsnd(msg_queue_id, &msg, sizeof(IPCMessage) - sizeof(long), 0);
    }
    msgctl(msg_queue_id, IPC_RMID, NULL);
}

void user_process(int process_id, int msg_queue_id)
{
    srand(time(NULL) + process_id);
    for (int i = 0; i < 10; i++) {
        // Générer une adresse virtuelle aléatoire
        int va = rand() % NUM_FRAMES;

        // Envoyer une requête au noyau
        IPCMessage msg = {1, process_id, va, 0, false};
        msgsnd(msg_queue_id, &msg, sizeof(IPCMessage) - sizeof(long), 0);


        // Recevoir la réponse
        msgrcv(msg_queue_id, &msg, sizeof(IPCMessage) - sizeof(long), process_id + 2, 0);

        printf("Process %d: VA 0x%x -> PA 0x%x [Page fault: %s]\n",
               process_id, va, msg.physical_address, msg.page_fault ? "Yes" : "No");
    }
}
