#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>

bool is_process_alive(const int pid, bool *is_alive);
bool is_kernel_process(const int pid, bool *is_kp);
bool is_user_process(const int pid, bool *is_up);

bool read_command_line(const int pid, char *buffer, unsigned int bsz);
bool read_imagepath(const int pid, char *buffer, unsigned int bsz);

void attach_process_by_pid(const int pid, bool *is_attached);
void detach_process_by_pid(const int pid);

bool read_memory(const int pid, unsigned long long start_address, unsigned long long end_address, unsigned char *memory);

struct VirtualMemoryArea
{
    unsigned long long  vm_start;
    unsigned long long  vm_end;
    unsigned char       permissions;
    unsigned long long  file_offset;
    unsigned char       device_major;
    unsigned char       device_minor;
    unsigned long long  inode;
    char*               pathname;
};
bool parse_maps_file(const int pid, struct VirtualMemoryArea **VMAs, int *vma_count);