# eBPF File Access and Page Cache Monitor

A C-based Linux observability project built with eBPF and libbpf to monitor file access patterns, storage activity, and page cache behavior in real time.

## Overview

This project is designed for Linux systems work and is intentionally kept in C so it stays approachable without needing Go or Rust. It focuses on showing how eBPF can be used to inspect file activity, process-level I/O behavior, and basic storage/memory diagnostics.

The goal is to create a practical starter project that demonstrates:
- file access tracing
- process-level I/O attribution
- basic page-cache awareness
- kernel-side monitoring with userspace reporting

## Why this project is a good fit

This is a strong intermediate-level Linux systems project because it combines:
- eBPF tracing
- kernel interaction
- file system behavior
- memory and cache awareness
- performance observability

It is especially useful for portfolio work, internal Linux projects, or a learning project around storage and BPF.

## Project goal

Build a lightweight Linux monitoring tool that lets you answer questions such as:
- Which processes are reading or writing the most files?
- Which files are being accessed the most?
- Are reads and writes concentrated on a few hot files?
- Is page cache behavior or writeback pressure likely affecting performance?

## Architecture

The project has two main parts:

1. Kernel-side BPF programs
   - attached to file-related kernel paths
   - collect metadata about file operations
   - store summaries in BPF maps

2. Userspace collector
   - reads BPF map data
   - aggregates the results
   - renders a readable terminal summary

## Features

- file open, read, and write tracing
- process and file attribution
- basic I/O byte accounting
- simple top-offender reporting
- lightweight Linux observability design

## Example output

```text
PID    COMM      PATH                                    OP       BYTES
1012   nginx     /var/log/nginx/access.log              write    8192
2234   python3   /tmp/cache.db                          read     65536
3031   postgres  /var/lib/postgresql/data/app.db        write    131072

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

## Prerequisites

This project is written in C and uses libbpf. You will need:

- Linux kernel with BPF support
- clang and llvm
- libbpf
- bpftool
- build-essential tools
- root privileges to load the eBPF program

Typical setup on Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential clang llvm libbpf-dev linux-headers-$(uname -r)
```

## Build

```bash
make
```

## Run

```bash
sudo ./scripts/run.sh
```

## Project status

This repository is a solid beginner-to-intermediate eBPF starter. It is intentionally designed to be clear and easy to extend without requiring Go or Rust.

Planned improvements:
- add better aggregation of file and process totals
- improve output formatting for terminal analysis
- add page-cache and writeback heuristics
- add more realistic testing workloads
- add documentation for kernel compatibility notes

## Why this is relevant

This project directly matches topics in Linux systems, kernel, storage, filesystem, and BPF discussions:
- file I/O monitoring
- dirty page visibility
- page cache behavior
- process-level storage analysis
- observability in modern Linux environments

## License

MIT

## Quick project summary

This project is meant to be a practical, portfolio-friendly Linux kernel tracing exercise using BPF, C, and libbpf. It is a good fit if you want to show real systems understanding without needing to learn an unfamiliar systems language like Go or Rust.
