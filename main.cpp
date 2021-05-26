#include "procfs_parser_api.h"

int main(int argc, char *argv[])
{
    int pid = 0;

    char *buffer = NULL;
    bool is_kp = false;

    buffer = (char *)malloc(4096);

    if (argc < 2)
    {
        fprintf(stderr, "You need to specify the Process ID\n");
        exit(EXIT_FAILURE);
    }

    pid = atoi(argv[1]);
    is_kernel_process(pid, is_kp);
    read_command_line(pid, buffer, 4096);
    read_imagepath(pid, buffer, 2);

    exit(EXIT_SUCCESS);
}
