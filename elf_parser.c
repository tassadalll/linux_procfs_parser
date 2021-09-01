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

void destroy_elf_data(struct elf_process* e_proc)
{
    if (e_proc == NULL) {
        return;
    }

    if (e_proc->hdr) {
        free(e_proc->hdr);
    }

    if (e_proc->phdr) {
        free(e_proc->phdr);
    }

    if (e_proc->vma_buffers) {
        int i;

        for (i = 0; i < e_proc->vma_count; i++) {
            if (e_proc->vma_buffers[i]) {
                free(e_proc->vma_buffers[i]);
            }
        }

        free(e_proc->vma_buffers);
    }

    free(e_proc);
}

bool parse_elf_header(struct elf_process* e_proc)
{
    bool result = true;

    unsigned char* elf_buffer = NULL;
    Elf64_Ehdr* elf_hdr64 = NULL;

    elf_hdr64 = malloc(sizeof(Elf64_Ehdr));
    NULLERRGOTO(elf_hdr64, result, done);

    elf_buffer = e_proc->vma_buffers[0];

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

    e_proc->hdr = elf_hdr64;
    elf_hdr64 = NULL;

done:

    if (elf_hdr64) {
        free(elf_hdr64);
    }

    return result;
}

bool parse_elf_program_header(struct elf_process* e_proc)
{
    bool result = false;

    unsigned char* cursor = NULL;
    int ph_relative_offset;
    int ph_idx;
    int i;

    e_proc->phdr = calloc(e_proc->hdr->e_phnum, sizeof(Elf64_Phdr));
    if (!e_proc->phdr) {
        return false;
    }

    for (i = 0; i < e_proc->vma_count - 1; i++) {
        if (e_proc->VMAs[i + 1].file_offset > e_proc->hdr->e_phoff) {
            ph_idx = i;
            break;
        }
    }

    ph_relative_offset = e_proc->hdr->e_phoff - e_proc->VMAs[ph_idx].file_offset;
    cursor = e_proc->vma_buffers[ph_idx] + ph_relative_offset;

    memcpy(e_proc->phdr, cursor, sizeof(Elf64_Phdr) * e_proc->hdr->e_phnum);

    return result;
}
