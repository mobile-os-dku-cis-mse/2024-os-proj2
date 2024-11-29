#pragma once

void kernel_process(int msg_queue_id, int *free_frames, int total_frames);
void user_process(int process_id, int msg_queue_id);
