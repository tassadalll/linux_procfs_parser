#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "procfs_parser_api.h"

void test1(const int pid)
{
    unsigned char* memory = NULL;
    int read_size = 0;
    memory = (unsigned char *)malloc(0x5000);
    if(!memory)
    {
        goto done;
    }
    memset(memory, 0x00, 0x5000);

    read_memory(pid, 0x558018303000, 0x558018308000, memory, 0x5000, &read_size);

done:

    if(memory)
    {
        free(memory);
    }
}

int main(int argc, char *argv[])
{
    bool result = false;
    int pid = 0;

    char *buffer = NULL;
    bool is_alive = false;
    bool is_kp = false;

    buffer = (char *)malloc(4096);

    if (argc < 2) {
        fprintf(stderr, "You need to specify the Process ID\n");
        exit(EXIT_FAILURE);
    }

    pid = atoi(argv[1]);

    result = is_process_alive(pid, &is_alive);
    if (result == false || is_alive == false) {
        goto done;
    }
    test1(pid);
    
    {
        struct VirtualMemoryArea* vma = NULL;
        int count;
        result = parse_maps_file(pid, &vma, &count );
        if(result) {
            
        }
    }
    

    is_kernel_process(pid, &is_kp);
    read_command_line(pid, buffer, 4096);
    read_imagepath(pid, buffer, 2);

done:

    if (buffer) {
        free(buffer);
    }

    exit(EXIT_SUCCESS);
}
