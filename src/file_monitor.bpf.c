// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "common.h"

struct pending_io {
    struct file_event event;
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 4096);
    __type(key, __u64); /* pid_tgid */
    __type(value, struct pending_io);
} pending SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);
} events SEC(".maps");

static __always_inline int start_io(struct file *file, __u8 op)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct pending_io io = {};
    struct file_event *event = &io.event;

    if (!file)
        return 0;

    event->pid = id >> 32;
    event->tid = (__u32)id;
    event->op = op;
    event->ts_ns = bpf_ktime_get_ns();
    bpf_get_current_comm(event->comm, sizeof(event->comm));

    if (bpf_d_path(&file->f_path, event->path, sizeof(event->path)) < 0)
        __builtin_memcpy(event->path, "<unavailable>", sizeof("<unavailable>"));

    bpf_map_update_elem(&pending, &id, &io, BPF_ANY);
    return 0;
}

static __always_inline int finish_io(long result)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct pending_io *io;
    struct file_event *event;

    io = bpf_map_lookup_elem(&pending, &id);
    if (!io)
        return 0;

    if (result > 0) {
        event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
        if (event) {
            __builtin_memcpy(event, &io->event, sizeof(*event));
            event->bytes = result;
            event->ts_ns = bpf_ktime_get_ns();
            bpf_ringbuf_submit(event, 0);
        }
    }

    bpf_map_delete_elem(&pending, &id);
    return 0;
}

SEC("kprobe/vfs_read")
int BPF_KPROBE(trace_vfs_read_enter, struct file *file)
{
    return start_io(file, OP_READ);
}

SEC("kretprobe/vfs_read")
int BPF_KRETPROBE(trace_vfs_read_exit, long result)
{
    return finish_io(result);
}

SEC("kprobe/vfs_write")
int BPF_KPROBE(trace_vfs_write_enter, struct file *file)
{
    return start_io(file, OP_WRITE);
}

SEC("kretprobe/vfs_write")
int BPF_KRETPROBE(trace_vfs_write_exit, long result)
{
    return finish_io(result);
}

char LICENSE[] SEC("license") = "GPL";
