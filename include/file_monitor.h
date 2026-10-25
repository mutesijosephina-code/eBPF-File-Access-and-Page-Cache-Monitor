#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H

#define MAX_PATH_LEN 256

struct file_event {
    __u32 pid;
    __u32 tid;
    __u64 ts_ns;
    __u64 bytes;
    __u8 op;
    char comm[16];
    char path[MAX_PATH_LEN];
};

#endif
