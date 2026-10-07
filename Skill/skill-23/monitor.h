#ifndef MONITOR_H
#define MONITOR_H

#include <sys/types.h>

typedef struct {
    long vm_size_kb;
    long vm_rss_kb;
    long user_cpu_ms;
    long sys_cpu_ms;
    int open_fds;
} ResourceSnapshot;

ResourceSnapshot get_resource_snapshot(pid_t pid);
void print_resource_snapshot(const ResourceSnapshot *s, const char *label);
double get_time_sec(void);

#endif /* MONITOR_H */
