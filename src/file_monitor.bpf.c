// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "common.h"

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, MAX_EVENTS);
    __type(key, __u64);
    __type(value, struct file_event);
} file_events SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 4096);
    __type(key, __u64);
    __type(value, __u64);
} file_bytes SEC(".maps");

static __always_inline void record_event(struct file *file, __u64 bytes, __u8 op)
{
    struct file_event event = {};
    struct path *path;
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    __u32 pid = pid_tgid >> 32;
    __u32 tid = (pid_tgid & 0xffffffff);

    if (!file)
        return;

    path = &file->f_path;
    event.pid = pid;
    event.tid = tid;
    event.ts_ns = bpf_ktime_get_ns();
    event.bytes = bytes;
    event.op = op;
    bpf_get_current_comm(event.comm, sizeof(event.comm));

    if (bpf_d_path(path, event.path, sizeof(event.path)) < 0) {
        __builtin_memcpy(event.path, "<unknown>", 10);
    }

    bpf_map_update_elem(&file_events, &pid, &event, BPF_ANY);

    __u64 key = ((__u64)pid << 32) | op;
    __u64 current = 0;
    __u64 *value = bpf_map_lookup_elem(&file_bytes, &key);
    if (value) {
        current = *value;
    }
    current += bytes;
    bpf_map_update_elem(&file_bytes, &key, &current, BPF_ANY);
}

SEC("kprobe/vfs_read")
int BPF_KPROBE(trace_vfs_read, struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    if (!file)
        return 0;

    record_event(file, count, OP_READ);
    return 0;
}

SEC("kprobe/vfs_write")
int BPF_KPROBE(trace_vfs_write, struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    if (!file)
        return 0;

    record_event(file, count, OP_WRITE);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
