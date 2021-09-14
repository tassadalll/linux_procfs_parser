#include <stdio.h>

#include "pp_internal.h"

long long get_file_size(FILE* file)
{
    long long offset = -1;
    off_t previous_offset;

    previous_offset = ftello(file);

    if (fseeko(file, 0, SEEK_END) == -1) {
        return -1;
    }

    offset = ftello(file);
    fseeko(file, previous_offset, SEEK_CUR);
    
    return offset;
}

