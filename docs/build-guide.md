# Build guide for this project

This guide explains how to build the `eBPF File Access and Page Cache Monitor` project in C using libbpf.

It is designed for a beginner-to-intermediate Linux systems developer who wants to build the project themselves.

## 1. What this project does

This project monitors file reads and writes in the Linux kernel using eBPF. It records:
- which process is doing the I/O
- which file is being accessed
- how many bytes are read or written
- basic operation metadata

The result is a simple but useful Linux observability tool.

## 2. Why C is the best choice

The best stack for this project is:
- C for userspace code
- libbpf for BPF integration
- Linux kernel with BPF support
- root privileges to load eBPF programs

This is a realistic and widely used approach in the Linux ecosystem.

## 3. Tools you need

On Ubuntu or Debian, install:

```bash
sudo apt update
sudo apt install -y build-essential clang llvm libbpf-dev linux-headers-$(uname -r) bpftool
```

You should also make sure `pkg-config` is available.

```bash
pkg-config --modversion libbpf
```

## 4. The project structure

Create this folder structure:

```text
eBPF-File-Access-and-Page-Cache-Monitor/
├── Makefile
├── README.md
├── src/
│   ├── common.h
│   ├── file_monitor.bpf.c
│   └── file_monitor.c
├── include/
│   └── file_monitor.h
├── scripts/
│   └── run.sh
├── docs/
│   └── architecture.md
└── .gitignore
```

## 5. Understanding the architecture

This project has two halves:

### Kernel side
The eBPF program runs in the kernel and attaches to file-related functions such as:
- `vfs_read`
- `vfs_write`

When these functions fire, the program records metadata like:
- PID
- process name
- file path
- byte count
- read/write operation

### Userspace side
The userspace program loads the BPF object and reads the aggregated results from BPF maps. It then prints a summary.

## 6. Step 1: create `src/common.h`

Create the file:

```c
#ifndef COMMON_H
#define COMMON_H

#define MAX_PATH_LEN 256
#define MAX_EVENTS 1024

#define OP_READ 1
#define OP_WRITE 2

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
```

This file defines the shared structure used by the BPF and userspace code.

## 7. Step 2: create `src/file_monitor.bpf.c`

This is the kernel-side BPF program.

```c
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
```

What this does:
- attaches to `vfs_read` and `vfs_write`
- gets the current process info
- resolves the file path
- stores the data in a BPF hash map

## 8. Step 3: create `include/file_monitor.h`

```c
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
```

This is the userspace-facing structure definition.

## 9. Step 4: create `src/file_monitor.c`

This is the userspace loader that loads the BPF object and keeps the program running.

```c
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

static volatile sig_atomic_t stop = 0;

static void handle_signal(int sig)
{
    (void)sig;
    stop = 1;
}

int main(void)
{
    struct bpf_object *obj;
    int err;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Loading eBPF file monitor...\n");

    obj = bpf_object__open_file("src/file_monitor.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }

    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        return 1;
    }

    printf("Monitor started. Press Ctrl+C to stop.\n");

    while (!stop) {
        sleep(1);
    }

    printf("Stopping monitor.\n");
    bpf_object__close(obj);
    return 0;
}
```

This is only the beginning. In a real version, you would also read BPF maps here and print results.

## 10. Step 5: create the `Makefile`

```make
CC ?= cc
CLANG ?= clang
CFLAGS ?= -O2 -g -Wall -Wextra

LIBBPF_CFLAGS := $(shell pkg-config --cflags libbpf 2>/dev/null)
LIBBPF_LDLIBS := $(shell pkg-config --libs libbpf 2>/dev/null)

BPF_OBJ = src/file_monitor.bpf.o
APP = src/file_monitor

all: $(BPF_OBJ) $(APP)

$(BPF_OBJ): src/file_monitor.bpf.c src/common.h
	$(CLANG) $(CFLAGS) $(LIBBPF_CFLAGS) -target bpf -c $< -o $@

$(APP): src/file_monitor.c include/file_monitor.h
	$(CC) $(CFLAGS) $(LIBBPF_CFLAGS) -Iinclude -o $@ $< $(LIBBPF_LDLIBS)

clean:
	rm -f $(BPF_OBJ) $(APP)

.PHONY: all clean
```

## 11. Step 6: create `scripts/run.sh`

```bash
#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "This project requires root privileges to load eBPF programs."
    exit 1
fi

if ! command -v pkg-config >/dev/null 2>&1; then
    echo "pkg-config is required. Install it first."
    exit 1
fi

if ! pkg-config --exists libbpf >/dev/null 2>&1; then
    echo "libbpf is not installed. Install it with:"
    echo "  sudo apt install libbpf-dev"
    exit 1
fi

make
./src/file_monitor
```

Make it executable:

```bash
chmod +x scripts/run.sh
```

## 12. Building the project

Run:

```bash
make
```

Then run as root:

```bash
sudo ./scripts/run.sh
```

## 13. Testing it

Open a second terminal and create some I/O activity:

```bash
dd if=/dev/zero of=/tmp/testfile bs=1M count=10 oflag=dsync
```

or:

```bash
for i in $(seq 1 1000); do echo "hello" >> /tmp/testlog.txt; done
```

This should trigger file writes and help you see the output.

## 14. Common beginner problems

### Problem: Permission denied
You need root access to load BPF programs.

```bash
sudo ./scripts/run.sh
```

### Problem: `libbpf` not found
Install:

```bash
sudo apt install libbpf-dev
```

### Problem: BPF object fails to load
This usually means:
- kernel headers are missing
- BPF support is not available
- you are using a kernel that blocks the program

### Problem: file path is not showing correctly
This often happens when the kernel hooks or the structure layout are not fully aligned. Keep the file structures simple and check all types carefully.

## 15. What to improve next

Once the basic version works, you can extend it with:
- file totals per process
- top files by byte count
- read vs write breakdowns
- latency tracking
- page cache or dirty-page heuristics
- a terminal table output

## 16. Good next milestone

The first milestone should be:
- trace `vfs_read` and `vfs_write`
- record PID and bytes
- show a live summary every second
- print a list of top readers and writers

That is enough to call the project a real working beginner-to-intermediate Linux eBPF project.

## 17. Final advice

This project is completely possible in C.
It is one of the best ways to learn Linux debugging, kernel tracing, and BPF concepts without needing Go or Rust.

The most important thing is to keep the scope realistic and build step by step.

## 18. Recommended learning order

1. Build a minimal eBPF program
2. Trace `vfs_read`
3. Trace `vfs_write`
4. Record PID and file path
5. Print aggregate I/O totals
6. Improve output formatting
7. Add page cache awareness
8. Add documentation and final polish

## 19. Summary

This project is a strong C-based eBPF project focused on:
- file access monitoring
- process-level I/O analysis
- storage and memory observability
- Linux systems engineering fundamentals

It is realistic, useful, and achievable without Go or Rust.
