#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>

bool is_process_alive(const int pid, bool &is_alive);
bool is_kernel_process(const int pid, bool &is_kp);
bool is_user_process(const int pid, bool &is_up);

bool read_command_line(const int pid, char *buffer, unsigned int bsz);
bool read_imagepath(const int pid, char *buffer, unsigned int bsz);

bool attach_process_by_pid(const int pid, bool &is_attached);
bool detach_process_by_pid(const int pid);