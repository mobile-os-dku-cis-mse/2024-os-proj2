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

    // Fork pour le noyau
    if (fork() == 0) {
        kernel_process(msg_queue_id, free_frames, total_frames);
        exit(0);
    }

    // Fork pour les processus utilisateurs
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (fork() == 0) {
            user_process(i, msg_queue_id);
            exit(0);
        }
    }

    // Attendre la fin des processus enfants
    for (int i = 0; i < NUM_PROCESSES + 1; i++) {
        wait(NULL);
    }

    // Nettoyage
    msgctl(msg_queue_id, IPC_RMID, NULL);

    return 0;
}
