#include <stdio.h>
#include "OS_memory.h"
#include "communication.h"
#include "process.h"

int main() {
    // Initialisation de l'OS
    int free_frames[NUM_FRAMES];
    int total_frames = initialize_os(free_frames, NUM_FRAMES);

    // Création de la file de messages
    int msg_queue_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    int fork_id = 0;

    msgctl(msg_queue_id, IPC_RMID, NULL);
    fork_id = fork();
    // Fork pour le noyau
    if (fork_id == 0) {
        kernel_process(msg_queue_id, free_frames, total_frames);
        exit(0);
    }
    if (fork_id == -1) {
        perror("parent fork");
        return 1;
    }

    // Fork pour les processus utilisateurs
    for (int i = 0; i < NUM_PROCESSES; i++) {
        fork_id = fork();
        if (fork_id == 0) {
            user_process(i, msg_queue_id);
            printf("Process %d finished\n", i);
            exit(0);
        }
        if (fork_id == -1)
            perror("parent fork");
    }
    return 0;
}
