#include <assert.h>

#include <wait.h>
#include <sys/ptrace.h>

#include "procfs_parser_api.h"

struct kernel_version
{
    int major;
    int minor;
    int patch;
};

#define VERSION_PREFIX "Linux version"
#define VERSION_PREFIX_LEN (sizeof("Linux version") - 1)

static bool read_kernel_version(struct kernel_version &kversion)
{
    bool result = false;

    FILE *vfile = NULL;
    char buffer[32] = "";
    char *cursor = NULL;
    struct kernel_version kv = {
        0,
    };

    vfile = fopen("/proc/version", "r");
    if (!vfile)
    {
        goto done;
    }

    if (fread(buffer, 1, sizeof(buffer), vfile) < sizeof(buffer))
    {
        goto done;
    }
    buffer[sizeof(buffer) - 1] = '\0';

    if (strncmp(buffer, VERSION_PREFIX, VERSION_PREFIX_LEN) != 0)
    {
        goto done;
    }

    cursor = buffer + VERSION_PREFIX_LEN;
    for (int i = 0; i < 3; i++)
    {
        long version;
        cursor++; // skip ' ' or '.'

        errno = 0;
        version = strtol(cursor, &cursor, 10);
        if (errno != 0)
        {
            goto done;
        }

        *((int *)&kv + i) = (int)version;
    }

    kversion = kv;
    result = true;

done:

    if (vfile != NULL)
    {
        fclose(vfile);
    }

    return result;
}

static int compare_kernel_version(struct kernel_version &kversion, const int major, const int minor, const int patch)
{
#define COMPARE_VERSION(v1, v2) \
    if (v1 != v2)           \
    {                           \
        return v1 - v2;         \
    }

    COMPARE_VERSION(kversion.major, major);
    COMPARE_VERSION(kversion.minor, minor);
    COMPARE_VERSION(kversion.patch, patch);

    return 0; // same kernel version
}

bool is_process_alive(const int pid, bool &is_alive)
{
    char path[32];

    if (sprintf(path, "/proc/%d", pid) < 0)
    {
        return false;
    }

    if (access(path, F_OK) == 0)
    {
        is_alive = true;
    }
    else
    {
        is_alive = false;
    }

    return true;
}

bool is_user_process(const int pid, bool &is_up)
{
    bool result = false;
    result = is_kernel_process(pid, is_up);
    if (result == true)
    {
        is_up = !is_up;
    }

    return result;
}

bool is_kernel_process(const int pid, bool &is_kp)
{
    bool result = false;
    char buffer[2];

    /* succeed to read at least 1 byte from cmdline file => user process */
    result = read_command_line(pid, buffer, sizeof(buffer));
    if (result == true)
    {
        is_kp = false;
    }

    return result;
}

bool read_command_line(const int pid, char *buffer, unsigned int bsz)
{
    bool result = false;

    char path[32];
    FILE *file = NULL;
    char *buff = NULL;
    int rsz = 0;

    if (sprintf(path, "/proc/%d/cmdline", pid) < 0)
    {
        return false;
    }

    buff = (char *)malloc(bsz);
    if (buff == NULL)
    {
        goto done;
    }
    memset(buff, 0, bsz);

    file = fopen(path, "r");
    if (file == NULL)
    {
        goto done;
    }

    rsz = fread(buff, 1, bsz, file);
    if (rsz == 0)
    {
        goto done;
    }

    strncpy(buffer, buff, bsz - 1);
    result = true;

done:

    if (file != NULL)
    {
        fclose(file);
    }

    if (buff != NULL)
    {
        free(buff);
    }

    return result;
}

bool read_imagepath(const int pid, char *buffer, unsigned int bsz)
{
    bool result = false;

    char exe_path[32];
    char buff[PATH_MAX] = "";
    int length;

    if (sprintf(exe_path, "/proc/%d/exe", pid) < 0)
    {
        return false;
    }

    length = readlink(exe_path, buff, sizeof(buff));
    if (length <= 0 || length + 1 < bsz) // fail or not enough buffer size
    {
        result = false;
        goto done;
    }
    else
    {
        char tmp_buff[PATH_MAX] = "";

        /* double check to determine if it's fileless process by realpath api */
        errno = 0;
        realpath(exe_path, tmp_buff); // return value consumed
        if (errno != 0 || errno != ENOENT)
        {
            result = false;
            goto done;
        }
        else if (errno == ENOENT)
        {
            bool fileless = false;
            static const int READLINK_DELETED_STR_LEN = sizeof(" (deleted)") - 1;

            if (length > READLINK_DELETED_STR_LEN)
            {
                char *cursor = buff + length - READLINK_DELETED_STR_LEN;
                if (strcmp(cursor, " (deleted)") == 0) // fileless process
                {
                    fileless = true;
                    *cursor = '\0';
                }
            }

            if (fileless == false)
            {
                result = false;
                goto done;
            }
        }
    }

    strcpy(buffer, buff);
    result = true;

done:

    return result;
}

bool attach_process_by_pid(const int pid, bool &is_attached)
{
    if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1 )
    {
        return false;
    }

    if(waitpid(pid, NULL, 0) == -1)
    {
        return true; // treat as success
    }

    return true;
}

bool detach_process_by_pid(const int pid)
{
    if(ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
    {
        return false;
    }

    return true;
}