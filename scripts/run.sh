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
