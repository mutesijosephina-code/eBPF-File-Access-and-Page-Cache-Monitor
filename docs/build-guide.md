# Build guide: C and libbpf

This guide walks through the working first milestone of the monitor. It is aimed at a learner comfortable with basic C and Linux terminal commands; no Go or Rust is needed.

## 1. Understand the event path

The project has a kernel-side BPF program and a userspace C program:

```text
vfs_read/vfs_write entry -> save process + file metadata
vfs_read/vfs_write return -> use actual return value (successful bytes)
BPF ring buffer -> userspace callback -> aggregate and print report
```

Entry probes alone cannot know how many bytes the operation actually transferred, so this implementation pairs each entry with a return probe. The BPF map named `pending` temporarily stores the entry data, keyed by thread ID. The `events` ring buffer carries completed events to userspace.

## 2. Install build dependencies

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential clang llvm bpftool pkg-config libbpf-dev libelf-dev zlib1g-dev
```

Check that the kernel exposes BTF:

```bash
ls -l /sys/kernel/btf/vmlinux
```

The Makefile uses `bpftool` to generate `src/vmlinux.h` from the running kernel's BTF. This header describes kernel types to the BPF compiler and is generated locally rather than committed.

## 3. Build the project

From the repository root:

```bash
make
```

The Makefile performs these steps:
1. Generates `src/vmlinux.h`.
2. Compiles `src/file_monitor.bpf.c` to a BPF object.
3. Generates `src/file_monitor.skel.h`, a libbpf loader interface.
4. Compiles the userspace C program.

If `make` reports a missing dependency, install the package named in the error. If it says kernel BTF is unavailable, the running kernel needs to provide BTF; this is common on standard distribution kernels but not every custom kernel.

## 4. Run safely

The monitor observes system-wide file activity, so use it only on a machine and workloads you are authorized to monitor. File names can be sensitive. Run it with the privileges required by your system:

```bash
sudo ./src/file_monitor
```

Stop using Ctrl+C. In another terminal, generate a small test workload:

```bash
cat /etc/hostname >/dev/null
printf 'hello\n' > /tmp/ebpf-monitor-test
cat /tmp/ebpf-monitor-test >/dev/null
```

Reports appear every five seconds and at shutdown. They show successful VFS bytes grouped by PID and operation, plus the last path observed for each row.

## 5. Read the shared event structure

`src/common.h` defines `struct file_event`, used by both BPF and userspace. Keep its field order and types in sync. The event includes PID/TID, timestamp, successful bytes, operation, command name, and a fixed-size path buffer.

`src/file_monitor.bpf.c` includes `vmlinux.h` before `common.h`, which provides Linux `__u*` integer types for the BPF side. The userspace source includes `<linux/types.h>` before the shared structure through the existing build environment; if your system needs it explicitly, add `#include <linux/types.h>` before `common.h`.

## 6. Understand the BPF code

- `pending` is a hash map keyed by `pid_tgid`. At probe entry the program stores the event details.
- `events` is a ring buffer shared with userspace.
- `start_io()` captures the process name and attempts to resolve a path.
- `finish_io()` ignores failed/zero-byte operations, copies successful results into the ring buffer, and removes the pending entry.
- Four probes pair entry and return for reads and writes.

The measured byte count is the return value from the VFS operation, not the requested count. This avoids counting failed operations or bytes that were not transferred.

## 7. Understand the userspace code

`src/file_monitor.c` opens, loads, and attaches the generated libbpf skeleton. It then polls the ring buffer. The callback aggregates counts by PID and operation in a small fixed-size array. Every five seconds, the program prints a table.

The starter implementation keeps the most recently observed file path per PID/operation row. It does not yet maintain a separate total for every file. This keeps the first milestone small enough to understand and extend.

## 8. Troubleshooting

### `bpftool` or `pkg-config` is missing
Install the packages listed above and retry `make`.

### `Kernel BTF is unavailable`
Check `/sys/kernel/btf/vmlinux`. Use a distribution kernel with BTF enabled or configure BTF support in your custom kernel.

### Permission or BPF load error
Run with `sudo` for initial testing. Some environments restrict BPF even for root (for example, containers or locked-down systems). Check kernel logs with `sudo dmesg | tail -50` for verifier or permission details.

### Probe attach fails
The running kernel may not expose `vfs_read`/`vfs_write` to kprobes, or may use different internal symbols. Check available symbols via `/proc/kallsyms` and consult the error output. A later portability improvement is to use supported tracepoints or fentry/fexit where available.

### No events appear
Generate file activity after starting the monitor. Some programs use different I/O paths; this initial version specifically observes VFS read/write functions.

## 9. Important measurement limits

This tool sees VFS-level operations. A successful read may be served from the page cache and a successful write may only enter buffered page cache. Consequently, the current measurements do not prove physical disk activity or calculate page-cache hit rates. Those require additional probes/metrics and careful correlation.

## 10. Suggested learning milestones

1. Build and run the existing monitor.
2. Read the shared event structure and trace one event through the code.
3. Add command-line reporting interval configuration.
4. Add latency measurement by storing entry timestamps and reporting duration.
5. Aggregate by file identity or path, with bounded map sizing.
6. Investigate page-cache/writeback metrics separately and document exactly what each metric means.

Build locally, test on a disposable workload, and commit each small change so you can easily undo experiments.
