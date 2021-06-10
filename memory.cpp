#include "procfs_parser_api.h"

bool read_memory(const int pid,
                 const unsigned long long start_address, const int size,
                 unsigned char *&memory)
{
    bool result = false;

    bool is_attached = false;
    char path[32];
    FILE *file = NULL;
    unsigned char *buffer = NULL;

    if (start_address <= 0 || size <= 0) {
        return false;
    }

    buffer = (unsigned char *)malloc(size);
    if (!buffer) {
        result = false;
        goto done;
    }

    if (sprintf(path, "/proc/%d/mem") < 0) {
        return false;
    }

    file = fopen(path, "rb");
    if (!file) {
        result = false;
        goto done;
    }

    if (fseek(file, (long)start_address, SEEK_SET) != 0) {
        result = false;
        goto done;
    }

    result = attach_process_by_pid(pid, is_attached);
    if (is_attached == false) {
        result = false; 
        goto done;
    }

    for (;;) {
        unsigned char rbuf[4096];
        int rsz;
        
        rsz = fread(rbuf, sizeof(rbuf), 0, SEEK_SET);
        if (rsz < 1) {
            break;
        }
    }

done:

    if (is_attached) {
        detach_process_by_pid(pid);
    }

    if (file) {
        fclose(file);
    }

    if (buffer) {
        free(buffer);
    }

    return result;
}
