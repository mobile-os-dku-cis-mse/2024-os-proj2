#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Taille d'une page mémoire
#define PAGE_SIZE 4096
#define NUM_FRAMES 64
#define NUM_PROCESSES 10
#define MSG_SIZE 256

// Structure pour les entrées de table de pages
typedef struct {
    int frame_number;
    bool valid;
} PageTableEntry;

// Structure pour une table de pages
typedef struct {
    PageTableEntry entries[NUM_FRAMES];
} PageTable;

// Structure pour le message IPC
typedef struct {
    long msg_type; // Type de message (identifie l'expéditeur)
    int process_id;
    int virtual_address;
    int physical_address;
    bool page_fault;
} IPCMessage;

int initialize_os(int *free_frames, int total_frames);
void initialize_page_table(PageTable *page_table);
void initialize_all_page_tables(PageTable page_tables[NUM_PROCESSES]);
