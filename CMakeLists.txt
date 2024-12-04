cmake_minimum_required(VERSION 3.22)
project(2024_os_proj2 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
add_definitions(-DPROJ2)
add_executable(scheduler
        src/main.c
        src/entity/scheduler.c
        src/util/process/process.c
        src/util/process/process.h
        src/entity/child.c
        src/entity/child.h
        src/util/pid_queue/pid_queue.c
        src/util/pid_queue/pid_queue.h
        src/util/timer/timer.c
        src/util/timer/timer.h
        src/util/msg_queue/message_queue.c
        src/util/msg_queue/message_queue.h
        src/entity/child.c
        src/entity/page_manager.c
        src/entity/page_manager.h
        src/util/shared_memory/shared_memory.c
        src/util/shared_memory/shared_memory.h
        #src/util/page_manager.c
        #src/util/page_manager.h
        #src/temp.c
)


add_executable(
        paging_test
        src/util/paging/test/paging_test.c
        src/util/tlb/tlb.c
        src/util/frame_list/frame_list.c
        src/util/paging/paging.c
)

add_executable(
        paging_test2
        src/util/paging/test/paging_test2.c
        src/util/tlb/tlb.c
        src/util/frame_list/frame_list.c
        src/util/paging/paging.c
)

add_executable(
        frame_list_test
        src/util/frame_list/frame_list.c
        src/util/frame_list/frame_list_test.c
)

add_executable(
        paging_test3
        src/util/paging/test/paging_test3.c
        src/util/tlb/tlb.c
        src/util/frame_list/frame_list.c
        src/util/paging/paging.c
        src/util/swap/swap.c
)

add_executable(
        paging_test4
        src/util/paging/test/paging_test4.c
        src/util/tlb/tlb.c
        src/util/frame_list/frame_list.c
        src/util/paging/paging.c
        src/util/swap/swap.c
)