#include "procfs_parser_api.h"

#include "list.h"

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


#define MAPS_LINE_CHK(ptr, expected_ch, label) \
    if (errno != 0 || *ptr != expected_ch) {   \
        goto label;                            \
    } else {                                   \
        ptr++;                                 \
    }

static bool parse_maps_line(const char *line, struct VirtualMemoryArea **parsed_vma)
{
    bool result = false;

    struct VirtualMemoryArea* vma = NULL;
    char* cursor = NULL;
    int remain_size;

    vma = malloc(sizeof(struct VirtualMemoryArea));
    if(!vma) {
        return false;
    }
    memset(vma, 0x00, sizeof(struct VirtualMemoryArea));

    errno = 0;
    vma->start_address = strtoull(line, &cursor, 16);
    MAPS_LINE_CHK(cursor, '-', done);
    
    vma->end_address = strtoull(cursor, &cursor, 16);
    MAPS_LINE_CHK(cursor, ' ', done);

    if( strlen(cursor) < 5 ) {
        goto done;
    }

    if (cursor[0] == 'r') {
        vma->permissions |= VMA_READ;
    }

    if (cursor[1] == 'w') {
        vma->permissions |= VMA_WRITE;
    }

    if (cursor[2] == 'x') {
        vma->permissions |= VMA_EXEC;
    }
    
    /* could be come 'p(=private)' or 's(=share)' */
    if (cursor[3] == 's') {
        vma->permissions |= VMA_MAYSHARE;
    }
    cursor += 5;

    vma->file_offset = strtoull(cursor, &cursor, 16);
    MAPS_LINE_CHK(cursor, ' ', done);

    vma->device_major = (unsigned char)strtol(cursor, &cursor, 16);
    MAPS_LINE_CHK(cursor, ':', done);
    
    vma->device_minor = (unsigned char)strtol(cursor, &cursor, 16);
    MAPS_LINE_CHK(cursor, ' ', done);

    vma->inode = strtoull(cursor, &cursor, 10);
    MAPS_LINE_CHK(cursor, ' ', done);

    /* skip whitespace */
    while(*cursor == ' ') {
        cursor++;
    }

    if(*cursor == '\n') {
        /* end of line, this VMA has no pathname */
    } else {
        int remain = strlen(cursor) - 1; // - newline
        strncpy(vma->pathname, cursor, remain);
        vma->pathname[remain] = '\0';
    }

    *parsed_vma = vma;
    vma = NULL;

    result = true;

done:

    if(vma) {
        free(vma);
    }

    return result;
}

bool parse_maps_file(const int pid, struct VirtualMemoryArea **parsed_vma, int *vma_count)
{
    bool result = false;

    struct VirtualMemoryArea* vma = NULL;
    FILE* file = NULL;
    pp_list list = NULL;
    char path[32];
    char buffer[512];
    int count;
    int i;

    if (!parsed_vma || !vma_count) {
        return false;
    }

    if (sprintf(path, "/proc/%d/maps", pid) < 0) {
        return false;
    }

    file = fopen(path, "r");
    if (!file) {
        goto done;
    }

    list = pp_list_create();
    if (!list) {
        goto done;
    }

    /* parse maps file line by line */
    while(fgets(buffer, sizeof(buffer), file) != NULL) {
        result = parse_maps_line(buffer, &vma);
        if (!result) {
            goto done;
        }

        result = pp_list_rpush( list, (void*)vma );
        if (!result) {
            goto done;
        }
        vma = NULL;
    }

    count = pp_list_size(list);
    vma = malloc(sizeof(struct VirtualMemoryArea) * count);
    if(!vma) {
        goto done;
    }

    for (i = 0; i < count; i++) {
        struct VirtualMemoryArea *tmp = NULL;

        result = pp_list_get(list, i, (void **)&tmp);
        if (!result) {
            goto done;
        }

        vma[i] = *tmp;
    }

    *vma_count = count;
    *parsed_vma = vma;
    vma = NULL;

done:

    if (list) {
        pp_list_destroy(list);
    }

    if (file) {
        fclose(file);
    }

    if (vma) {
        free(vma);
    }

    return result;
}