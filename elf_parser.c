#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "pp_internal.h"
#include "elf_parser.h"

struct elf_process* create_elf_data(int pid, struct VirtualMemoryArea* VMAs, int vma_count)
{
    bool result = true;

    struct elf_process* process = NULL;
    int i;

    process = calloc(1, sizeof(struct elf_process));
    NULLERRGOTO(process, result, done);

    process->pid = pid;
    process->VMAs = VMAs;
    process->vma_count = vma_count;

    process->vma_buffers = calloc(vma_count, sizeof(unsigned char*));
    NULLERRGOTO(process->vma_buffers, result, done);

    for (i = 0; i < 5; i++) {
        process->vma_buffers[i] = dump_process_memory_by_address(pid, VMAs[i].start_address, VMAs[i].end_address);
        NULLERRGOTO(process->vma_buffers[i], result, done);
    }

done:

    if (result == false) {
        destroy_elf_data(process);
        process = NULL;
    }

    return process;
}

void destroy_elf_data(struct elf_process* process)
{
    if (process == NULL) {
        return;
    }

    if (process->hdr) {
        free(process->hdr);
    }

    if (process->phdr) {
        free(process->phdr);
    }

    if (process->vma_buffers) {
        int i;

        for (i = 0; i < process->vma_count; i++) {
            if (process->vma_buffers[i]) {
                free(process->vma_buffers[i]);
            }
        }

        free(process->vma_buffers);
    }

    free(process);
}

bool parse_elf_header(struct elf_process* process)
{
    bool result = true;

    unsigned char* elf_buffer = NULL;
    Elf64_Ehdr* elf_hdr64 = NULL;

    elf_hdr64 = malloc(sizeof(Elf64_Ehdr));
    NULLERRGOTO(elf_hdr64, result, done);

    elf_buffer = process->vma_buffers[0];

    if (memcmp(elf_buffer, ELFMAG, SELFMAG) != 0) {
        SETERRGOTO(result, done);
    }

    if (elf_buffer[EI_CLASS] == ELFCLASS32) {
        Elf32_Ehdr* hdr32 = (Elf32_Ehdr*)elf_buffer;

        /* convert ELF 32 to 64 */
        memcpy(elf_hdr64, hdr32, offsetof(Elf32_Ehdr, e_entry));
        elf_hdr64->e_entry = (Elf64_Off)hdr32->e_entry;
        elf_hdr64->e_phoff = (Elf64_Off)hdr32->e_phoff;
        elf_hdr64->e_shoff = (Elf64_Off)hdr32->e_shoff;
        memcpy(&elf_hdr64->e_flags, &hdr32->e_flags, sizeof(Elf32_Ehdr) - offsetof(Elf32_Ehdr, e_flags));
    }
    else { // = ELFCLASS64
        memcpy(elf_hdr64, elf_buffer, sizeof(Elf64_Ehdr));
    }

    process->hdr = elf_hdr64;
    elf_hdr64 = NULL;

done:

    if (elf_hdr64) {
        free(elf_hdr64);
    }

    return result;
}

bool parse_elf_program_header(struct elf_process* process)
{
    bool result = false;

    unsigned char* cursor = NULL;
    int ph_relative_offset;
    int ph_idx;
    int i;

    process->phdr = calloc(process->hdr->e_phnum, sizeof(Elf64_Phdr));
    if (process->phdr == NULL) {
        return false;
    }

    for (i = 0; i < process->vma_count - 1; i++) {
        if (process->VMAs[i + 1].file_offset > process->hdr->e_phoff) {
            ph_idx = i;
            break;
        }
    }

    ph_relative_offset = process->hdr->e_phoff - process->VMAs[ph_idx].file_offset;
    cursor = process->vma_buffers[ph_idx] + ph_relative_offset;

    memcpy(process->phdr, cursor, sizeof(Elf64_Phdr) * process->hdr->e_phnum);

    return result;
}
