#ifndef __ELF_PARSER__
#define __ELF_PARSER__

#include <stdbool.h>
#include <elf.h>

#include "pp_list.h"

typedef void* elf_process_t;

elf_process_t create_elf_data(int pid, pp_list_t VMAs);

void destroy_elf_data(elf_process_t e_process);

bool parse_elf_header(elf_process_t e_process);

bool parse_elf_program_header(elf_process_t e_process);

Elf64_Ehdr* get_elf_header(elf_process_t e_process);

Elf64_Phdr* get_elf_program_header(elf_process_t e_process);

#endif
