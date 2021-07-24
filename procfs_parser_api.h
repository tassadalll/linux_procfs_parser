#ifndef __PROCFS_PARSER_API__
#define __PROCFS_PARSER_API__

#include <stdbool.h>
#include <linux/limits.h>

bool is_process_alive(const int pid, bool *is_alive);
bool is_kernel_process(const int pid, bool *is_kp);
bool is_user_process(const int pid, bool *is_up);

bool read_command_line(const int pid, char *buffer, unsigned int bsz);
bool read_imagepath(const int pid, char *buffer, unsigned int bsz);

void attach_process_by_pid(const int pid, bool *is_attached);
void detach_process_by_pid(const int pid);

bool read_memory(const int pid, unsigned long long start_address, unsigned long long end_address,
                 unsigned char *memory, int size, int *read_size);

/* Virtual Memory Area permissions */
#define VMA_READ     0x1
#define VMA_WRITE    0x2
#define VMA_EXEC     0x4
#define VMA_MAYSHARE 0x8

struct VirtualMemoryArea
{
    unsigned long long  start_address;
    unsigned long long  end_address;
    unsigned char       permissions;
    unsigned long long  file_offset;
    unsigned char       device_major;
    unsigned char       device_minor;
    unsigned long long  inode;
    char                pathname[PATH_MAX];
};
bool parse_maps_file(const int pid, struct VirtualMemoryArea **VMAs, int *vma_count);

#endif // __PROCFS_PARSER_API__