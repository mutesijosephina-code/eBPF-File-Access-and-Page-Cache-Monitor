# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

CC ?= cc
CLANG ?= clang
BPFTOOL ?= bpftool
CFLAGS ?= -O2 -g -Wall -Wextra

LIBBPF_CFLAGS := $(shell pkg-config --cflags libbpf 2>/dev/null)
LIBBPF_LDLIBS := $(shell pkg-config --libs libbpf 2>/dev/null)
MULTIARCH := $(shell $(CC) -print-multiarch 2>/dev/null)
BPF_ARCH := $(shell case "$(shell uname -m)" in x86_64) echo x86;; aarch64) echo arm64;; armv7*) echo arm;; s390x) echo s390;; ppc64le) echo powerpc;; riscv64) echo riscv;; *) echo "unsupported";; esac)

VMLINUX_H = src/vmlinux.h
BPF_OBJ = src/file_monitor.bpf.o
SKEL = src/file_monitor.skel.h
APP = src/file_monitor

.PHONY: all clean check-deps
all: check-deps $(APP)

check-deps:
	@command -v $(CLANG) >/dev/null || { echo "clang is required"; exit 1; }
	@command -v $(BPFTOOL) >/dev/null || { echo "bpftool is required"; exit 1; }
	@pkg-config --exists libbpf || { echo "libbpf development files are required (for example: libbpf-dev)"; exit 1; }
	@test "$(BPF_ARCH)" != "unsupported" || { echo "Unsupported CPU architecture: $(shell uname -m)"; exit 1; }

$(VMLINUX_H):
	@test -r /sys/kernel/btf/vmlinux || { echo "Kernel BTF is unavailable at /sys/kernel/btf/vmlinux"; exit 1; }
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > $@

$(BPF_OBJ): src/file_monitor.bpf.c src/common.h $(VMLINUX_H)
	$(CLANG) -g -O2 -target bpf -D__TARGET_ARCH_$(BPF_ARCH) -I src $(LIBBPF_CFLAGS) $(if $(MULTIARCH),-I/usr/include/$(MULTIARCH)) -c $< -o $@

$(SKEL): $(BPF_OBJ)
	$(BPFTOOL) gen skeleton $< > $@

$(APP): src/file_monitor.c src/common.h $(SKEL)
	$(CC) $(CFLAGS) $(LIBBPF_CFLAGS) -Isrc -Iinclude -o $@ src/file_monitor.c $(LIBBPF_LDLIBS)

clean:
	rm -f $(BPF_OBJ) $(SKEL) $(APP) $(VMLINUX_H)
