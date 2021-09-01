#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <wait.h>

#include "procfs_parser_api.h"
#include "pp_internal.h"

struct kernel_version {
    int major;
    int minor;
    int patch;
};

#define VERSION_PREFIX "Linux version"
#define VERSION_PREFIX_LEN ( sizeof( "Linux version" ) - 1 )

static bool read_kernel_version(struct kernel_version* kversion)
{
    bool result = true;

    FILE* vfile = NULL;
    char buffer[32] = "";
    char* cursor = NULL;
    struct kernel_version kv = {
        0,
    };

    vfile = fopen("/proc/version", "r");
    if (!vfile) {
        SETERRGOTO(result, done);
    }

    if (fread(buffer, 1, sizeof(buffer), vfile) < sizeof(buffer)) {
        SETERRGOTO(result, done);
    }
    buffer[sizeof(buffer) - 1] = '\0';

    if (strncmp(buffer, VERSION_PREFIX, VERSION_PREFIX_LEN) != 0) {
        SETERRGOTO(result, done);
    }

    cursor = buffer + VERSION_PREFIX_LEN;
    for (int i = 0; i < 3; i++) {
        long version;
        cursor++; // skip ' ' or '.'

        errno = 0;
        version = strtol(cursor, &cursor, 10);
        if (errno != 0) {
            SETERRGOTO(result, done);
        }

        *((int*)&kv + i) = (int)version;
    }

    *kversion = kv;
    result = true;

done:

    if (vfile != NULL) {
        fclose(vfile);
    }

    return result;
}

#define COMPARE_VERSION(v1, v2) \
    if (v1 != v2) {             \
        return v1 - v2;         \
    }

static int compare_kernel_version(struct kernel_version* kversion,
    const int major, const int minor, const int patch)
{
    COMPARE_VERSION(kversion->major, major);
    COMPARE_VERSION(kversion->minor, minor);
    COMPARE_VERSION(kversion->patch, patch);

    return 0; // same kernel version
}

bool is_process_alive(const int pid, bool* is_alive)
{
    char path[32];

    if (sprintf(path, "/proc/%d", pid) < 0) {
        return false;
    }

    if (access(path, F_OK) == 0) {
        *is_alive = true;
    }
    else {
        *is_alive = false;
    }

    return true;
}

bool is_user_process(const int pid, bool* is_up)
{
    bool result = false;

    result = is_kernel_process(pid, is_up);
    if (result == true) {
        *is_up = !*is_up;
    }

    return result;
}

bool is_kernel_process(const int pid, bool* is_kp)
{
    bool result = false;
    char buffer[2];

    /* succeed to read at least 1 byte from cmdline file => user process */
    result = read_command_line(pid, buffer, sizeof(buffer));
    if (result == true) {
        *is_kp = false;
    }

    return result;
}

bool read_command_line(const int pid, char* cmdline, unsigned int bsz)
{
    bool result = false;

    char path[32];
    FILE* file = NULL;
    char* buff = NULL;
    int rsz = 0;

    if (sprintf(path, "/proc/%d/cmdline", pid) < 0) {
        return false;
    }

    buff = (char*)malloc(bsz);
    if (buff == NULL) {
        SETERRGOTO(result, done);
    }
    memset(buff, 0, bsz);

    file = fopen(path, "r");
    if (file == NULL) {
        SETERRGOTO(result, done);
    }

    rsz = fread(buff, 1, bsz, file);
    if (rsz == 0) {
        SETERRGOTO(result, done);
    }

    strncpy(cmdline, buff, bsz - 1);
    result = true;

done:

    if (file) {
        fclose(file);
    }

    if (buff) {
        free(buff);
    }

    return result;
}

bool read_imagepath(const int pid, char* imagepath, unsigned int bsz)
{
    bool result = false;

    char exe_path[32];
    char buffer[PATH_MAX] = "";
    int length;

    if (sprintf(exe_path, "/proc/%d/exe", pid) < 0) {
        return false;
    }

    errno = 0;
    length = readlink(exe_path, buffer, sizeof(buffer));
    if (errno != 0 && errno != ENOENT) {
        return false;
    }

    /* failed or not enough buffer size */
    if (length <= 0 || length + 1 > bsz) {
        return false;
    }
    else {
        static const int READLINK_DELETED_STR_LEN = sizeof(" (deleted)") - 1;

        if (length > READLINK_DELETED_STR_LEN) {
            char* cursor = buffer + length - READLINK_DELETED_STR_LEN;

            /* check if it is fileless process or UPX process */
            if (strcmp(cursor, " (deleted)") == 0) {
                *cursor = '\0';
            }
        }
    }

    strcpy(imagepath, buffer);
    result = true;

done:

    return result;
}

void attach_process_by_pid(const int pid, bool* is_attached)
{
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        *is_attached = false;
        return;
    }

    *is_attached = true;

    waitpid(pid, NULL, 0);
}

void detach_process_by_pid(const int pid)
{
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) != -1) {
        // failed to detach, but cannot do nothing
    }
}
