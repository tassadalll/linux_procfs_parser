#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "procfs_parser_api.h"
#include "pp_internal.h"
#include "elf_parser.h"
#include "list.h"

static int read_memory(const int pid, FILE* memory_file, unsigned char* buffer, int size)
{
    int rsz = -1;

    if (attach_process_by_pid(pid) == true) {
        rsz = fread(buffer, 1, size, memory_file);

        detach_process_by_pid(pid);
    }

    return rsz;
}

int read_process_memory(const int pid, unsigned long long start_address, unsigned char* memory, int size)
{
    FILE* file = NULL;
    char path[32] = "";
    int rsz = -1;

    if (size <= 0) {
        return -1;
    }

    if (sprintf(path, "/proc/%d/mem", pid) < 0) {
        return -1;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        goto done;
    }

    if (fseek(file, (long)start_address, SEEK_SET) != 0) {
        goto done;
    }

    rsz = read_memory(pid, file, memory, size);

done:

    if (file) {
        fclose(file);
    }

    return rsz;
}

int read_process_memory_by_address(const int pid, unsigned long long start_address, unsigned long long end_address, unsigned char* memory)
{
    return read_process_memory(pid, start_address, memory, (int)(end_address - start_address));
}

unsigned char* dump_process_memory(const int pid, unsigned long long start_address, int size)
{
    unsigned char* memory = NULL;
    int rsz;

    if (size <= 0) {
        return NULL;
    }

    memory = malloc(size);
    if (memory == NULL) {
        return NULL;
    }

    rsz = read_process_memory(pid, start_address, memory, size);
    if (rsz < size) {
        /* fail if less than the requested size is read */
        free(memory);
        memory = NULL;
    }

    return memory;
}

unsigned char* dump_process_memory_by_address(const int pid, unsigned long long start_address, unsigned long long end_address)
{
    return dump_process_memory(pid, start_address, (int)(end_address - start_address));
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

    struct VirtualMemoryArea* VMAs = NULL;
    unsigned char* buffer = NULL;
    int vma_count = 0;
    int size = 0;
    int i;

    result = parse_maps_file(pid, &VMAs, &vma_count);
    IFERRGOTO(result, done);

    for (i = 0; i < vma_count; i++) {
        // TODO: dump stack of each thread( VMA with pathname "[stack:\d+]" ) 
        // only the main thread stack is being dumped

        if (strcmp(vma[i].pathname, "[stack]") == 0) {
            break;
        }
    }

    if (i == vma_count) {
        SETERRGOTO(result, done); // failed to find stack area
    }

    size = (int)(VMAs[i].end_address - VMAs[i].start_address);

    buffer = dump_process_memory_by_size(pid, VMAs[i].start_address, size);
    if (buffer == NULL) {
        SETERRGOTO(result, done);
    }

    *stack = buffer;
    *stack_size = size;

done:

    return result;
}

static bool select_vma_by_inode(unsigned long long inode, struct VirtualMemoryArea* VMAs, int vma_count, pp_list selected_VMAs)
{
    bool result = true;
    int i;

    for (i = 0; i < vma_count; i++) {
        struct VirtualMemoryArea* cursor = &VMAs[i];

        if (cursor->inode == inode) {
            result = pp_list_rpush(selected_VMAs, cursor);
            if (result == false) {
                break;
            }
        }
    }

    return result;
}

static bool dump_process_image_ex(const int pid, pp_list VMAs, const char* dump_path)
{
    bool result = false;

    struct VirtualMemoryArea* vma = NULL;
    int vma_count;
    int i;
    unsigned char* buffer = NULL;
    FILE* dump_file = NULL;
    int to_wsz;
    int wsz;

    vma_count = pp_list_size(VMAs);
    if (vma_count <= 0) {
        SETERRGOTO(result, done);
    }

    dump_file = fopen(dump_path, "wb");
    NULLERRGOTO(dump_file, result, done);

    result = pp_list_get(VMAs, 0, (void**)&vma);
    IFERRGOTO(result, done);

    to_wsz = (int)(vma->end_address - vma->start_address);
    buffer = dump_process_memory_by_size(pid, vma->start_address, to_wsz);
    NULLERRGOTO(buffer, result, done);

    wsz = fwrite(buffer, 1, to_wsz, dump_file);
    if (wsz < to_wsz) {
        SETERRGOTO(result, done);
    }

    free(buffer);
    buffer = NULL;

    for (i = 1; i < vma_count; i++) {
        result = pp_list_get(VMAs, i, (void**)&vma);
        IFERRGOTO(result, done);

        buffer = dump_process_memory_by_address(pid, vma->start_address, vma->end_address);
        NULLERRGOTO(buffer, result, done);
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
    NULLERRGOTO(image_VMAs, result, done);

    result = select_vma_by_inode(image_inode, VMAs, vma_count, image_VMAs);
    IFERRGOTO(result, done);

    elf_proc = create_elf_data(pid, VMAs, vma_count);
    NULLERRGOTO(elf_proc, result, done);

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
        SETERRGOTO(result, done);              \
    } else {                                   \
        ptr++;                                 \
    }

static bool parse_maps_line(const char* line, struct VirtualMemoryArea** parsed_vma)
{
    bool result = true;

    struct VirtualMemoryArea* vma = NULL;
    char* cursor = NULL;
    int remain_size;

    vma = malloc(sizeof(struct VirtualMemoryArea));
    NULLERRGOTO(vma, result, done);
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

done:

    if (vma) {
        free(vma);
    }

    return result;
}

bool parse_maps_file(const int pid, struct VirtualMemoryArea** parsed_VMAs, int* vma_count)
{
    bool result = true;

    struct VirtualMemoryArea* VMAs = NULL;
    struct VirtualMemoryArea* vma = NULL;
    FILE* file = NULL;
    pp_list list = NULL;
    char path[32];
    char buffer[512];
    int count;
    int i;

    if (sprintf(path, "/proc/%d/maps", pid) < 0) {
        return false;
    }

    file = fopen(path, "r");
    NULLERRGOTO(file, result, done);

    list = pp_list_create();
    NULLERRGOTO(list, result, done);

    /* parse maps file line by line */
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        result = parse_maps_line(buffer, &vma);
        IFERRGOTO(result, done);

        result = pp_list_rpush(list, (void*)vma);
        IFERRGOTO(result, done);

        vma = NULL;
    }

    count = pp_list_size(list);
    VMAs = malloc(sizeof(struct VirtualMemoryArea) * count);
    NULLERRGOTO(VMAs, result, done);

    for (i = 0; i < count; i++) {
        result = pp_list_lpop(list, (void**)&vma);
        IFERRGOTO(result, done);

        VMAs[i] = *vma;
        vma = NULL;
    }

    *vma_count = count;
    *parsed_VMAs = VMAs;
    VMAs = NULL;

done:

    if (list) {
        pp_list_destroy_with_nodes(list, NULL);
    }

    if (file) {
        fclose(file);
    }

    if (VMAs) {
        free(VMAs);
    }

    if (vma) {
        free(vma);
    }

    return result;
}
