# eBPF File Access and Page Cache Monitor

A Linux observability project built with eBPF to monitor file access patterns, storage activity, and page cache behavior in real time.

## Overview

This project helps developers, system administrators, and Linux performance engineers understand:
- which processes are reading and writing files
- which files are the hottest I/O targets
- how I/O latency changes under system load
- whether page cache behavior and memory pressure are contributing to slowdowns

The goal is to provide a practical, low-overhead tracing tool that connects application behavior with kernel storage and memory activity.

## Why this project matters

Linux systems often show poor storage performance without obvious causes. A process may appear to be slow, but the actual bottleneck may be related to:
- page cache efficiency
- writeback pressure
- dirty-page accumulation
- high file access churn
- filesystem access patterns

This project focuses on making those relationships visible using eBPF.

## Project goals

- Trace file open, read, write, and close activity using eBPF
- Attribute I/O to specific processes and files
- Measure file operation frequency and latency
- Observe page cache and writeback-related indicators
- Provide a real-time summary through a CLI or lightweight dashboard
- Build a useful project aligned with Linux storage, filesystem, memory, and BPF work

## Scope

This project covers:
- eBPF tracing of file operations
- process-level and file-level I/O attribution
- page cache awareness and basic writeback indicators
- user-space aggregation and reporting

## Non-goals

- Full filesystem implementation
- Full enterprise observability platform integration
- Distributed tracing across machines
- Full support for every possible kernel variant

## Architecture

The project is split into two parts:

### 1. eBPF programs
The kernel-side code traces relevant file and storage events and stores summary information in BPF maps.

### 2. Userspace collector
The userspace side reads the aggregated BPF data, summarizes it, and prints clear output for debugging and performance analysis.

## Features

- Process-centric file monitoring
- File path and operation tracking
- Read/write throughput and latency summaries
- Top offenders by process and file
- Page cache and writeback visibility
- Simple CLI based reporting

## Example output

```text
PID    COMM      FILE                                  OP       BYTES    LATENCY(ms)
1012   nginx     /var/log/nginx/access.log            write    8192     2.8
2234   python3   /tmp/cache.db                        read     65536    4.1
3031   postgres  /var/lib/postgresql/data/app.db      write    131072   9.6

Page cache: moderate
Dirty pages: elevated
Writeback pressure: active
Top I/O hotspot: /var/lib/postgresql/data/app.db
```

## Repository structure

```text
eBPF-File-Access-and-Page-Cache-Monitor/
├── README.md
├── Makefile
├── LICENSE
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

## Requirements

A recent Linux system with:
- kernel headers
- clang and llvm
- libbpf
- bpftool
- a modern kernel with BPF support

Recommended:
- Ubuntu 22.04+ or Fedora 38+
- root privileges for loading BPF programs

## Build instructions

```bash
make
```

## Run

```bash
sudo ./scripts/run.sh
```

## Development idea

This project is intentionally structured as a realistic intermediate Linux systems project. It demonstrates:
- BPF tracing
- kernel interaction
- process and file attribution
- storage and page cache diagnostics
- systems engineering and observability work

## Project status

This repository currently contains the project foundation and eBPF skeleton for a file and page-cache monitor. It is designed to be extended with:
- more detailed file metrics
- latency aggregation
- page cache heuristics
- dashboard output
- synthetic workloads for testing

## License

MIT

## Suggested next steps

1. Extend the BPF map with aggregated counters by file and process
2. Add a user-space summary renderer
3. Track I/O latency histograms
4. Add basic page-cache and writeback heuristics
5. Validate with realistic file I/O workloads

---

## GitHub-ready project proposal

### Title
`eBPF File Access and Page Cache Monitor for Linux Storage and Memory Diagnostics`

### Abstract
This project builds a lightweight Linux observability tool using eBPF to monitor file access patterns, storage activity, and page cache behavior in real time. The aim is to help developers and system administrators understand which processes are reading or writing files, how much I/O is occurring, and whether memory pressure or page cache behavior is affecting system performance.

### Problem
Modern Linux workloads often experience storage slowdowns without obvious root causes. In many systems, performance issues are influenced by a mix of I/O activity, filesystem access patterns, writeback pressure, and memory state. Traditional tools often show coarse-grained metrics but do not easily reveal the relationship between application behavior and kernel storage dynamics.

### Objectives
- Monitor file opens, reads, writes, and close events using eBPF
- Track which processes generate the most file I/O
- Measure latency and throughput per file and process
- Estimate page cache behavior and identify signs of writeback pressure
- Present results in readable output for debugging and optimization

### Technical approach
The project uses eBPF programs attached to relevant kernel hooks and tracepoints to collect file and I/O metadata. A userspace collector then reads the aggregated data from BPF maps and summarizes it for visualization and analysis.

### Expected outcome
By the end of the project, the repository will contain a working Linux tracing tool that demonstrates BPF-based monitoring, file I/O observation, and basic page-cache diagnostics.

## Contributors

This project can be used as a portfolio item for Linux systems, BPF, kernel, or observability work.
