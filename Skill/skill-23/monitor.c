#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include "monitor.h"

double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

ResourceSnapshot get_resource_snapshot(pid_t pid) {
    ResourceSnapshot snap;
    memset(&snap, 0, sizeof(snap));

    char path[64];

    /* 1. Read /proc/<pid>/status */
    snprintf(path, sizeof(path), "/proc/%d/status", pid == 0 ? getpid() : pid);
    FILE *f = fopen(path, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmSize:", 7) == 0) {
                sscanf(line + 7, "%ld", &snap.vm_size_kb);
            } else if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line + 6, "%ld", &snap.vm_rss_kb);
            }
        }
        fclose(f);
    }

    /* 2. Read /proc/<pid>/stat */
    snprintf(path, sizeof(path), "/proc/%d/stat", pid == 0 ? getpid() : pid);
    f = fopen(path, "r");
    if (f) {
        unsigned long utime = 0, stime = 0;
        char comm[64]; int dummy_d; char dummy_c;
        if (fscanf(f, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu",
                   &dummy_d, comm, &dummy_c, &dummy_d, &dummy_d, &dummy_d, &dummy_d, &dummy_d,
                   (unsigned int *)&dummy_d, &utime, &utime, &utime, &utime, &utime, &stime) >= 15) {
            long ticks_per_sec = sysconf(_SC_CLK_TCK);
            if (ticks_per_sec > 0) {
                snap.user_cpu_ms = (long)(utime * 1000 / ticks_per_sec);
                snap.sys_cpu_ms = (long)(stime * 1000 / ticks_per_sec);
            }
        }
        fclose(f);
    }

    /* 3. Count open file descriptors in /proc/<pid>/fd */
    snprintf(path, sizeof(path), "/proc/%d/fd", pid == 0 ? getpid() : pid);
    DIR *dir = opendir(path);
    if (dir) {
        struct dirent *de;
        int count = 0;
        while ((de = readdir(dir)) != NULL) {
            if (de->d_name[0] != '.') count++;
        }
        closedir(dir);
        snap.open_fds = count;
    }

    return snap;
}

void print_resource_snapshot(const ResourceSnapshot *s, const char *label) {
    printf("  [Resource Monitor: %s]\n", label);
    printf("    VmSize: %ld KB | VmRSS: %ld KB | Open FDs: %d | CPU User: %ld ms, Sys: %ld ms\n",
           s->vm_size_kb, s->vm_rss_kb, s->open_fds, s->user_cpu_ms, s->sys_cpu_ms);
}
