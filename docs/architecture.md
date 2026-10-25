# eBPF File Access and Page Cache Monitor

## Architecture overview

This project follows a simple eBPF architecture:

1. Kernel tracepoints and kprobes fire when file operations happen.
2. The eBPF programs capture process, file, and byte-count metadata.
3. BPF maps aggregate events and counters.
4. The userspace program reads and summarizes the data.
5. The output shows which files and processes are most active.

## Typical event flow

```text
Application -> VFS -> eBPF hook -> BPF map -> userspace -> summary output
```

## Planned extensions

- per-file read/write totals
- latency breakdown per operation
- page cache and writeback heuristics
- Grafana-compatible metrics output
- file path normalization and grouping

## Example metrics to add

- bytes read per second
- bytes written per second
- top read files
- top write files
- average latency by process
- dirty-page pressure estimate
