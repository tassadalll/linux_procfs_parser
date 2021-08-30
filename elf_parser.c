#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pp_internal.h"
#include "elf_parser.h"

struct elf_process* create_elf_data(int pid, struct VirtualMemoryArea* VMAs, int vma_count)
{
    struct elf_process* e_proc = NULL;

    e_proc = calloc(1, sizeof(struct elf_process));
    if (!e_proc) {
        return NULL;
    }

    e_proc->pid = pid;
    e_proc->VMAs = VMAs;
    e_proc->vma_count = vma_count;

    e_proc->vma_buffer = calloc(vma_count, sizeof(unsigned char*));
    if(!e_proc->vma_buffer) {
        free(e_proc);
        return NULL;
    }

    return e_proc;
}

void destroy_elf_data(struct elf_process* e_proc)
{
    if (!e_proc) {
        return;
    }

    if (e_proc->phdr) {
        free(e_proc);
    }

    if (e_proc->vma_buffer) {
        int i;

        for (i = 0; i < e_proc->vma_count; i++) {
            if (e_proc->vma_buffer[i]) {
                free(e_proc->vma_buffer[i]);
            }
        }

        free(e_proc->vma_buffer);
    }

    free(e_proc);
}

bool parse_elf_header(struct elf_process* e_proc)
{
    bool result = false;
    unsigned char* elf_buffer = NULL;

    Elf32_Ehdr* hdr32 = NULL;
    Elf64_Ehdr* hdr64 = NULL;

    if (e_proc->vma_buffer[0] == NULL) {
        result = read_memory_by_address(e_proc->pid, e_proc->VMAs[0].start_address, e_proc->VMAs[0].end_address, &e_proc->vma_buffer[0]);
        if (!result) {
            goto done;
        }
    }

    elf_buffer = e_proc->vma_buffer[0];

    if (memcmp(elf_buffer, ELFMAG, SELFMAG) != 0) {
        ERRGOTO(result, done);
    }

    if (elf_buffer[EI_CLASS] == ELFCLASS32) {
        hdr32 = (Elf32_Ehdr*)elf_buffer;
    }
    else { // = ELFCLASS64
        hdr64 = (Elf64_Ehdr*)elf_buffer;
    }

done:

    return result;
}

// bool parse_elf_program_header(unsigned char* buffer, int bsz, Elf64_Phdr* phdr)
// {
//     bool result = false;

//     return result;
// }