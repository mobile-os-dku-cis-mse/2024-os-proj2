#pragma once

#include "common.h"
#include "process.h"
#include "queue.h"
#include "virtual_memory.h"

void initialize_scheduling_log(const char *filename);
void initialize_virtual_log(const char *filename);
void initialize_disk_log(const char *filename);
void log_memory_state(const int va, const int pa, Node *node, bool first_table_fault, bool second_table_fault, int data, int idx, char *request, bool isSwap, bool isCow);
void cal_wait_time(Node *current_node);
void cal_turn_time(Node *current_node);
const char* get_timestamp();
void log_process_event(const Process *process, const Node *current, const char *event);
void log_queue_state(const char *queue_name, const Queue *queue);
void close_scheduling_log();
void close_virtual_log();