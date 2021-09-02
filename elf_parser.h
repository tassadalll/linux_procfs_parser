#ifndef __ELF_PARSER__
#define __ELF_PARSER__

#include <stdbool.h>

#include <elf.h>
#include <sys/types.h>

#include "procfs_parser_api.h"

struct elf_process
{
    int pid;

    struct VirtualMemoryArea* VMAs;
    unsigned char** vma_buffers;
    int vma_count;

    bool is_elf32;
    Elf64_Ehdr* hdr;
    Elf64_Phdr* phdr;
};

struct elf_process* create_elf_data(int pid, struct VirtualMemoryArea* VMAs, int vma_count);

void destroy_elf_data(struct elf_process* e_proc);

bool parse_elf_header(struct elf_process* e_proc);

bool parse_elf_program_header(struct elf_process* e_proc);

#endif
