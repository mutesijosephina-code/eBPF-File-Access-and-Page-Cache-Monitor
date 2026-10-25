# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

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
