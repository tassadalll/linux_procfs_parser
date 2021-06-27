#include "procfs_parser_api.h"

static bool read_memory_ex(const int pid, FILE *memfile, unsigned char *memory, int memsize)
{
    bool result = false;

    bool is_attached = false;
    unsigned char *buffer = NULL;
    int rsz = 0;

    buffer = (unsigned char *)malloc(memsize);
    if (!buffer) {
        result = false;
        goto done;
    }

    /* if the attach fails, `/proc/[pid]/mem` file cannot be read */
    attach_process_by_pid(pid, &is_attached);
    if (is_attached == false) {
        result = false;
        goto done;
    }

    rsz = fread(buffer, 1, memsize, memfile);
    if (rsz < memsize) {
        result = false;
        goto done;
    }

    memcpy(memory, buffer, memsize);

done:

    if(is_attached) {
        detach_process_by_pid(pid);
    }

    if(buffer) {
        free(buffer);
    }

    return result;
}

bool read_memory(const int pid,
                 unsigned long long start_address, unsigned long long end_address,
                 unsigned char *memory)
{
    bool result = false;
    
    char path[32];
    bool is_attached = false;
    FILE *file = NULL;
    int rsize = (int)(end_address - start_address);

    if (start_address <= 0 || rsize <= 0) {
        return false;
    }

    if (sprintf(path, "/proc/%d/mem", pid) < 0) {
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

    result = read_memory_ex(pid, file, memory, rsize);
    if(!result) {
        goto done;
    }

done:

    if (file) {
        fclose(file);
    }

    return result;
}

bool parse_maps_file(const int pid, struct VirtualMemoryArea **parsed_vma, int *vma_count)
{
    bool result = false;

    struct VirtualMemoryArea* vma = NULL;
    FILE* file = NULL;
    char path[32];
    char buffer[512];

    if (sprintf(path, "/proc/%d/maps", pid) < 0) {
        return false;
    }

    file = fopen(path, "r");
    if(!file) {
        return false;
    }

    while(fgets(buffer, sizeof(buffer), file) != NULL) {

    }




done:

    if(file) {
        fclose(file);
    }

    if (vma) {
        free(vma);
    }

    return result;
}