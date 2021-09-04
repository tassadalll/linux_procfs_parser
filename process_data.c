#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "procfs_parser_api.h"
#include "pp_internal.h"

#define SZ_STATUS_RB (4096)

inline static char* get_next_char_pointer(char* ptr, char ch)
{
    char* cursor = ptr;

    while(cursor++) {
        if (*cursor == '\0') {
            cursor = NULL;
            break;
        }
        else if (*cursor == ch) {
            break;
        }
    }

    return cursor;
}

bool parse_process_stat(const int pid, struct ProcessStat* process_stat)
{
    bool result = true;
    FILE* file = NULL;
    char buffer[SZ_STATUS_RB] = "";
    char path[32] = "";
    char* cursor = NULL;

    if( sprintf(path, "/proc/%d/stat", pid) < 0 ) {
        SETERRGOTO(result, done);
    }

    file = fopen(path, "r");
    NULLERRGOTO(file, result, done);

    cursor = fgets(buffer, sizeof(buffer), file);
    NULLERRGOTO(cursor, result, done);

    fclose(file);
    file = NULL;

    process_stat->pid = atoi(buffer);
    cursor = get_next_char_pointer(cursor, ' ');
    
    cursor = strrchr(cursor, ')');
    cursor = get_next_char_pointer(cursor, ' ');
    cursor = get_next_char_pointer(cursor, ' ');
    process_stat->cstime = strtoull(cursor, &cursor, 10);



done:

    if (file) {
        fclose(file);
    }
    
    return result;
}