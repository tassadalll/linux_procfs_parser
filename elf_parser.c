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
    Elf64_Ehdr* hdr64 = NULL;

    hdr64 = malloc(sizeof(Elf64_Ehdr));
    NULLERRGOTO(hdr64, result, done);

    elf_buffer = process->vma_buffers[0];

    if (memcmp(elf_buffer, ELFMAG, SELFMAG) != 0) {
        /* not the frontmost ELF image VMA, invalid VMA */
        SETERRGOTO(result, done);
    }

    if (elf_buffer[EI_CLASS] == ELFCLASS32) {
        Elf32_Ehdr* hdr32 = (Elf32_Ehdr*)elf_buffer;

        /* convert ELF 32 to 64 */
        memcpy(hdr64, hdr32, offsetof(Elf32_Ehdr, e_entry));
        hdr64->e_entry = hdr32->e_entry;
        hdr64->e_phoff = hdr32->e_phoff;
        hdr64->e_shoff = hdr32->e_shoff;
        memcpy(&hdr64->e_flags, &hdr32->e_flags, sizeof(Elf32_Ehdr) - offsetof(Elf32_Ehdr, e_flags));

        process->is_elf32 = true;
    }
    else { // = ELFCLASS64
        memcpy(hdr64, elf_buffer, sizeof(Elf64_Ehdr));

        process->is_elf32 = false;
    }

    process->hdr = hdr64;
    hdr64 = NULL;

done:

    if (hdr64) {
        free(hdr64);
    }

    return result;
}


bool parse_elf_program_header(struct elf_process* process)
{
    bool result = false;

    Elf64_Phdr* phdr64 = NULL;
    unsigned char* cursor = NULL;
    int ph_relative_offset;
    int ph_idx;
    int i;

    if (process->hdr == NULL) {
        result = parse_elf_header(process);
        IFERRGOTO(result, done);
        if(result == false) {
            return false;
        }
    }

    phdr64 = calloc(process->hdr->e_phnum, sizeof(Elf64_Phdr));
    NULLERRGOTO(phdr64, result, done);

    for (i = 0; i < process->vma_count - 1; i++) {
        if (process->VMAs[i + 1].file_offset > process->hdr->e_phoff) {
            ph_idx = i;
            break;
        }
    }

    /* convert virtual address to relative offset */
    ph_relative_offset = process->hdr->e_phoff - process->VMAs[ph_idx].file_offset;
    cursor = process->vma_buffers[ph_idx] + ph_relative_offset;

    if(process->is_elf32) {
        Elf32_Phdr* phdr32 = (Elf32_Phdr*)cursor;

        for (i = 0; i < process->vma_count; i++) {
            phdr64[i].p_type   = phdr32[i].p_type;
            phdr64[i].p_flags  = phdr32[i].p_flags;
            phdr64[i].p_offset = phdr32[i].p_offset;
            phdr64[i].p_vaddr  = phdr32[i].p_vaddr;
            phdr64[i].p_paddr  = phdr32[i].p_paddr;
            phdr64[i].p_filesz = phdr32[i].p_filesz;
            phdr64[i].p_memsz  = phdr32[i].p_memsz;
            phdr64[i].p_align  = phdr32[i].p_align;
        }
    } else {
        memcpy(phdr64, cursor, sizeof(Elf64_Phdr) * process->hdr->e_phnum);
    }

    process->phdr = phdr64;
    phdr64 = NULL;

done:

    if (phdr64) {
        free(phdr64);
    }

    return result;
}
