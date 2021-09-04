#ifndef __PROCFS_PARSER_API__
#define __PROCFS_PARSER_API__

#include <stdbool.h>
#include <linux/limits.h>
#include <sys/types.h>

bool is_process_alive(const int pid, bool* is_alive);
bool is_kernel_process(const int pid, bool* is_kp);
bool is_user_process(const int pid, bool* is_up);

bool read_command_line(const int pid, char* cmdline, unsigned int bsz);
bool read_imagepath(const int pid, char* imagepath, unsigned int bsz);

bool attach_process_by_pid(const int pid);
void detach_process_by_pid(const int pid);

int read_process_memory(const int pid, unsigned long long start_address, unsigned char* memory, int size);
int read_process_memory_by_address(const int pid, unsigned long long start_address, unsigned long long end_address, unsigned char* memory);
unsigned char* dump_process_memory(const int pid, unsigned long long start_address, int size);
unsigned char* dump_process_memory_by_address(const int pid, unsigned long long start_address, unsigned long long end_address);

bool dump_process_image(const int pid, unsigned char** image, int* img_size);
bool dump_process_stack(const int pid, unsigned char** stack, int* stack_size);

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

/* parse "/proc/[pid]/maps" */
bool parse_maps_file(const int pid, struct VirtualMemoryArea** VMAs, int* vma_count);

struct ProcessStat
{
    pid_t pid;
    char comm[NAME_MAX];
    char state;
    pid_t ppid;
    gid_t pgrp;
    int session;
    int tty_nr;
    gid_t tpgid;
    unsigned int flags;
    unsigned long long minflt;
    unsigned long long cminflt;
    unsigned long long majflt;
    unsigned long long cmajflt;
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long cutime;
    unsigned long long cstime;
    long long priority;
    long long nice;
    long long num_threads;
    long long itrealvalue;
    unsigned long long starttime;
    unsigned long long vsize;
    long long rss;
    unsigned long long rsslim;
    unsigned long long startcode;
    unsigned long long endcode;
    unsigned long long startstack;
    unsigned long long kstkesp;
    unsigned long long kstkeip;
    unsigned long long signal;
    unsigned long long blocked;
    unsigned long long sigignore;
    unsigned long long sigcatch;
    unsigned long long wchan;
    unsigned long long nswap;
    unsigned long long cnswap;
    int exit_signal;
    int processor;
    unsigned int rt_priority;
    unsigned int policy;
    unsigned long long delayacct_blkio_ticks;
    unsigned long long guest_time;
    unsigned long long cguest_time;
    unsigned long long start_data;
    unsigned long long end_data;
    unsigned long long start_brk;
    unsigned long long arg_start;
    unsigned long long arg_end;
    unsigned long long env_start;
    unsigned long long env_end;
    int exit_code;
};

/* parse "/proc/[pid]/stat" */
bool parse_process_stat(const int pid, struct ProcessStat* process_stat);


#endif // __PROCFS_PARSER_API__