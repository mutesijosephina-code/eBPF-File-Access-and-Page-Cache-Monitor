#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "This script needs root privileges to load eBPF programs."
    exit 1
fi

make

./src/file_monitor
