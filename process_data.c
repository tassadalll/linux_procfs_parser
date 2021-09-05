#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

#define ERRSTRTOULL( prev, next )                                 \
    if ((unsigned long long)cursor == (unsigned long long)next) { \
        SETERRGOTO(result, done);                                 \
    } else {                                                      \
        prev = next;                                              \
    }

bool parse_process_stat(const int pid, struct ProcessStat* stat)
{
    bool result = true;

    FILE* file = NULL;
    char buffer[SZ_STATUS_RB] = "";
    char path[32] = "";
    char* cursor = NULL;
    char* next = NULL;
    int len;

    if (sprintf(path, "/proc/%d/stat", pid) < 0) {
        SETERRGOTO(result, done);
    }

    file = fopen(path, "r");
    NULLERRGOTO(file, result, done);

    cursor = fgets(buffer, sizeof(buffer), file);
    NULLERRGOTO(cursor, result, done);

    fclose(file);
    file = NULL;

    stat->pid = (pid_t)atoi(cursor);

    cursor = strchr(cursor, '(');
    cursor++;
    next = strrchr(cursor, ')');

    len = (unsigned long long)next - (unsigned long long)cursor;
    strncpy(stat->comm, cursor, len);
    stat->comm[len] = '\0';

    cursor = next + 2; // skip ") "
    stat->state = *cursor;
    cursor++;

    stat->ppid = (pid_t)strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->pgrp = (gid_t)strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->session = (int)strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->tty_nr = (int)strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->tpgid = (gid_t)strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->flags = (unsigned int)strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->minflt = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->cminflt = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->majflt = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->cmajflt = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->utime = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->stime = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->cutime = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->cstime = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->priority = strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->nice = strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->num_threads = strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->itrealvalue = strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->starttime = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->vsize = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->rss = strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->rsslim = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->startcode = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->endcode = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->startstack = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->kstkesp = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->kstkeip = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->signal = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->blocked = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->sigignore = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->sigcatch = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->wchan = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->nswap = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->cnswap = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->exit_signal = (int)strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->processor = (int)strtoll(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->rt_priority = (unsigned int)strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->policy = (unsigned int)strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->delayacct_blkio_ticks = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->guest_time = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->cguest_time = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->start_data = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->end_data = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->start_brk = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->arg_start = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->arg_end = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->env_start = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->env_end = strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

    stat->exit_code = (int)strtoull(cursor, &next, 10);
    ERRSTRTOULL(cursor, next);

done:

    if (file) {
        fclose(file);
    }

    return result;
}