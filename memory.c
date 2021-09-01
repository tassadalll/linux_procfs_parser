#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "procfs_parser_api.h"
#include "pp_internal.h"
#include "elf_parser.h"
#include "list.h"

static int read_memory(const int pid, unsigned long long start_address, unsigned char** memory, int memsize)
{
    bool result = false;

    FILE* file = NULL;
    char path[32] = "";
    bool is_attached = false;
    unsigned char* buffer = NULL;
    int read_size = 0;

    if (memory == NULL || start_address < 0 || memsize <= 0) {
        return false;
    }

    buffer = (unsigned char*)malloc(memsize);
    if (!buffer) {
        return false;
    }

    if (sprintf(path, "/proc/%d/mem", pid) < 0) {
        SETERRGOTO(result, done);
    }

    file = fopen(path, "rb");
    if (!file) {
        SETERRGOTO(result, done);
    }

    /* if the attach fails, `/proc/[pid]/mem` file cannot be read */
    attach_process_by_pid(pid, &is_attached);
    if (is_attached == false) {
        SETERRGOTO(result, done);
    }

    if (fseek(file, (long)start_address, SEEK_SET) != 0) {
        SETERRGOTO(result, done);
    }

    read_size = fread(buffer, 1, memsize, file);
    if (read_size < memsize) {
        SETERRGOTO(result, done);
    }

    *memory = buffer;
    buffer = NULL;

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

    return read_size;
}

int read_memory_by_size(const int pid, unsigned long long start_address, int to_rsz, unsigned char** memory)
{
    return read_memory(pid, start_address, memory, to_rsz);
}

int read_memory_by_address(const int pid,
    unsigned long long start_address, unsigned long long end_address, unsigned char** memory)
{
    int memsize = (int)(end_address - start_address);

    return read_memory(pid, start_address, memory, memsize);
}

static bool search_inode_by_imagepath(struct VirtualMemoryArea* vma, int vma_count, const char* image_path, unsigned long long* inode)
{
    bool found = false;
    int i;

    for (i = 0; i < vma_count; i++) {
        if (strcmp(vma[i].pathname, image_path) == 0) {
            *inode = vma[i].inode;
            found = true;
            break;
        }
    }

    return found;
}

bool dump_process_stack(const int pid, unsigned char** stack, int* stack_size)
{
    bool result = false;

    struct VirtualMemoryArea* vma = NULL;
    int vma_count = 0;
    char* memory = NULL;
    int size = 0;
    int i;

    result = parse_maps_file(pid, &vma, &vma_count);
    IFERRGOTO(result, done);

    for (i = 0; i < vma_count; i++) {
        if (strcmp(vma[i].pathname, "[stack]") == 0) {
            break;
        }
    }

    if (i == vma_count) {
        SETERRGOTO(result, done); // failed to find stack area
    }

    *stack_size = read_memory_by_address(pid, vma[i].start_address, vma[i].end_address, stack);
    if (*stack_size <= 0) {
        SETERRGOTO(result, done);
    }

done:

    return result;
}

static bool select_vma_by_inode(unsigned long long inode, struct VirtualMemoryArea* vma, int vma_count, pp_list selected_VMAs)
{
    bool result = true;
    int i;

    for (i = 0; i < vma_count; i++) {
        struct VirtualMemoryArea* cursor = &vma[i];

        if (cursor->inode == inode) {
            result = pp_list_rpush(selected_VMAs, cursor);
            if (!result) {
                break;
            }
        }
    }

    return result;
}

static bool dump_process_image_ex(const int pid, pp_list vma_list, const char* dump_path)
{
    bool result = false;

    struct VirtualMemoryArea* vma = NULL;
    int vma_count;
    int i;
    unsigned char* buffer = NULL;
    FILE* dump_file = NULL;
    int to_wsz;
    int wsz;

    dump_file = fopen(dump_path, "wb");
    if (dump_file == NULL) {
        SETERRGOTO(result, done);
    }

    vma_count = pp_list_size(vma_list);
    if (vma_count <= 0) {
        SETERRGOTO(result, done);
    }

    result = pp_list_get(vma_list, 0, (void**)&vma);
    IFERRGOTO(result, done);

    to_wsz = (int)(vma->end_address - vma->start_address);
    result = read_memory_by_size(pid, vma->start_address, to_wsz, &buffer);
    IFERRGOTO(result, done);

    to_wsz = (int)(vma->end_address - vma->start_address);
    wsz = fwrite(buffer, 1, to_wsz, dump_file);
    if (wsz < to_wsz) {
        SETERRGOTO(result, done);
    }

    free(buffer);
    buffer = NULL;

    for (i = 1; i < vma_count; i++) {
        result = pp_list_get(vma_list, i, (void**)&vma);
        IFERRGOTO(result, done);

        result = read_memory_by_address(pid, vma->start_address, vma->end_address, &buffer);
        if (!result) {
            goto done;
        }
    }

done:

    if (buffer) {
        free(buffer);
    }

    if (dump_file) {
        fclose(dump_file);
    }

    return result;
}

bool dump_process_image(const int pid, const char* dump_path)
{
    bool result = false;

    char image_path[PATH_MAX] = "";
    struct VirtualMemoryArea* VMAs = NULL;
    int vma_count = 0;
    unsigned long long image_inode = 0;
    struct elf_process* elf_proc = NULL;
    pp_list image_VMAs = NULL;
    bool found = false;
    int i;

    result = parse_maps_file(pid, &VMAs, &vma_count);
    IFERRGOTO(result, done);

    result = read_imagepath(pid, image_path, sizeof(image_path));
    IFERRGOTO(result, done);

    found = search_inode_by_imagepath(VMAs, vma_count, image_path, &image_inode);
    if (found == false || image_inode == UNKNOWN_INODE) {
        fprintf(stderr, "Cannot find process image area\n");
        SETERRGOTO(result, done);
    }

    image_VMAs = pp_list_create();
    if (image_VMAs == NULL) {
        SETERRGOTO(result, done);
    }

    result = select_vma_by_inode(image_inode, VMAs, vma_count, image_VMAs);
    if (!result) {
        goto done;
    }

    elf_proc = create_elf_data(pid, VMAs, vma_count);
    if (!elf_proc) {
        SETERRGOTO(result, done);
    }

    parse_elf_header(elf_proc);
    parse_elf_program_header(elf_proc);

done:

    if (elf_proc) {
        destroy_elf_data(elf_proc);
    }

    if (VMAs) {
        free(VMAs);
    }

    if (image_VMAs) {
        pp_list_destroy(image_VMAs);
    }

    return result;
}

#define MAPS_LINE_CHK(ptr, expected_ch, label) \
    if (errno != 0 || *ptr != expected_ch) {   \
        goto label;                            \
    } else {                                   \
        ptr++;                                 \
    }

static bool parse_maps_line(const char* line, struct VirtualMemoryArea** parsed_vma)
{
    bool result = false;

    struct VirtualMemoryArea* vma = NULL;
    char* cursor = NULL;
    int remain_size;

    vma = malloc(sizeof(struct VirtualMemoryArea));
    if (!vma) {
        return false;
    }
    memset(vma, 0x00, sizeof(struct VirtualMemoryArea));

    errno = 0;
    vma->start_address = strtoull(line, &cursor, 16);
    MAPS_LINE_CHK(cursor, '-', done);

    vma->end_address = strtoull(cursor, &cursor, 16);
    MAPS_LINE_CHK(cursor, ' ', done);

    if (strlen(cursor) < 5) {
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
    while (*cursor == ' ') {
        cursor++;
    }

    if (*cursor == '\n') {
        /* end of line, this VMA has no pathname */
    }
    else {
        int remain = strlen(cursor) - 1; // - newline
        strncpy(vma->pathname, cursor, remain);
        vma->pathname[remain] = '\0';
    }

    *parsed_vma = vma;
    vma = NULL;

    result = true;

done:

    if (vma) {
        free(vma);
    }

    return result;
}

bool parse_maps_file(const int pid, struct VirtualMemoryArea** parsed_vma, int* vma_count)
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
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        result = parse_maps_line(buffer, &vma);
        if (!result) {
            goto done;
        }

        result = pp_list_rpush(list, (void*)vma);
        if (!result) {
            goto done;
        }
        vma = NULL;
    }

    count = pp_list_size(list);
    vma = malloc(sizeof(struct VirtualMemoryArea) * count);
    if (!vma) {
        goto done;
    }

    for (i = 0; i < count; i++) {
        struct VirtualMemoryArea* tmp = NULL;

        result = pp_list_get(list, i, (void**)&tmp);
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
