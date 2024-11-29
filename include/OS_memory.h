#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PAGE_SIZE 4096            // Taille d'une page (4 KB)
#define PHYSICAL_MEMORY_SIZE 65536 // Taille mémoire physique totale (64 KB)
#define NUM_FRAMES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE) // Nombre de cadres de page
#define NUM_PROCESSES 10          // Nombre de processus utilisateur
#define PAGE_TABLE_SIZE 1024      // Taille de la table de pages (1K entrées)

typedef struct {
    int frame_number;  // Numéro du cadre de page (PFN)
    bool valid;        // Flag : true si l'entrée est valide, false sinon
} PageTableEntry;

typedef struct {
    PageTableEntry entries[PAGE_TABLE_SIZE]; // Table de pages (tableau d'entrées)
} PageTable;

typedef struct {
    int free_frames[NUM_FRAMES]; // Liste des cadres de page libres
    int free_frame_count;        // Nombre de cadres libres
} OSState;
