# eBPF File Access and Page Cache Monitor

A C and libbpf learning project that observes successful VFS read/write operations and reports totals by process and operation. It is an initial working milestone toward broader Linux storage and memory observability.

> **Scope note:** This first implementation measures bytes returned by `vfs_read`/`vfs_write`. That is not the same as physical disk I/O: reads may be served from the page cache, and writes may be buffered. Page-cache hit/miss and writeback-pressure metrics are future work, not currently reported by this version.

## What it does

- Attaches eBPF entry/return probes to `vfs_read` and `vfs_write`.
- Captures the process ID/name, operation, successful byte count, and a best-effort file path.
- Sends completed events through a BPF ring buffer.
- Aggregates operation counts and bytes by process and operation in userspace.
- Prints a summary every five seconds and once more when stopped.

This is a system-wide monitor. File paths may contain sensitive information; only run it on systems and workloads you are authorized to observe.

## Requirements

- Linux with BPF support and kernel BTF at `/sys/kernel/btf/vmlinux`.
- A supported architecture: x86_64, arm64, arm, s390x, ppc64le, or riscv64.
- `clang`, `llvm`, `bpftool`, `pkg-config`, a C compiler, and libbpf development headers.
- Root privileges (or suitable BPF capabilities) to load and attach probes.

Ubuntu/Debian example:

```bash
sudo apt update
sudo apt install -y build-essential clang llvm bpftool pkg-config libbpf-dev libelf-dev zlib1g-dev
```

If your running kernel does not provide `/sys/kernel/btf/vmlinux`, use a kernel with BTF enabled. The project generates `src/vmlinux.h` from that file when building.

## Build and run

From the repository root:

```bash
make
sudo ./src/file_monitor
```

Stop with Ctrl+C. A typical report looks like:

```text
PID      OP      COMMAND          OPERATIONS   BYTES      LAST FILE SEEN
--------------------------------------------------------------------------------
1234     read    cat              3            8192       /etc/hostname
1234     write   dd               1            1048576    /tmp/sample
```

Generate test activity in another terminal:

```bash
cat /etc/hostname >/dev/null
printf 'sample data\n' > /tmp/ebpf-monitor-test
cat /tmp/ebpf-monitor-test >/dev/null
```

Clean generated build files with `make clean`.

## How the code is organized

- `src/file_monitor.bpf.c`: eBPF probes, pending-operation map, and ring buffer.
- `src/file_monitor.c`: libbpf skeleton loader, ring-buffer reader, and report aggregation.
- `src/common.h`: event structure shared between BPF and userspace.
- `Makefile`: generates `vmlinux.h` and a libbpf skeleton, then builds the monitor.
- `docs/build-guide.md`: beginner-friendly walkthrough and implementation notes.

## Limitations and next steps

- `vfs_read`/`vfs_write` kprobe availability and signatures vary by kernel; attach may fail on kernels that hide or change these functions.
- The file path is best-effort and may be truncated or unavailable.
- This version summarizes by PID and operation, retaining the most recently observed path for each row.
- Ring-buffer events can be lost under very high event rates.
- It does not yet measure physical block-device I/O, page-cache hits/misses, dirty pages, or writeback pressure.

Possible extensions: latency histograms, grouping by file identity, syscall tracepoints for broader coverage, and separately sourced memory/writeback statistics.

## License

See [LICENSE](LICENSE).
