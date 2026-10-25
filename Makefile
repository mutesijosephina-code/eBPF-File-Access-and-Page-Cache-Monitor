# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

CFLAGS = -O2 -g -Wall -Wextra
CLANG ?= clang
LLC ?= llc
LLVM_STRIP ?= llvm-strip

BPF_OBJ = src/file_monitor.bpf.o
USER_OBJ = src/file_monitor

all: $(BPF_OBJ) $(USER_OBJ)

$(BPF_OBJ): src/file_monitor.bpf.c src/common.h
	$(CLANG) $(CFLAGS) -target bpf -c $< -o $@

$(USER_OBJ): src/file_monitor.c include/file_monitor.h
	$(CC) -O2 -g -Wall -Wextra -o $@ $< -lbpf

clean:
	rm -f $(BPF_OBJ) $(USER_OBJ)

.PHONY: all clean
