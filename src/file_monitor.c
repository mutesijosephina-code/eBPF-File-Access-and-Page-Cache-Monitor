// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <bpf/libbpf.h>
#include "common.h"
#include "file_monitor.skel.h"

#define MAX_STATS 256
#define REPORT_INTERVAL_SECONDS 5

struct process_stat {
    __u32 pid;
    __u8 op;
    __u64 bytes;
    __u64 operations;
    char comm[16];
    char path[MAX_PATH_LEN];
};

static volatile sig_atomic_t exiting;
static struct process_stat stats[MAX_STATS];
static size_t stats_count;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    exiting = 1;
}

static int handle_event(void *context, void *data, size_t data_size)
{
    const struct file_event *event = data;
    size_t i;

    (void)context;
    if (data_size < sizeof(*event)) {
        fprintf(stderr, "Received a truncated event\n");
        return 0;
    }

    for (i = 0; i < stats_count; i++) {
        if (stats[i].pid == event->pid && stats[i].op == event->op)
            break;
    }

    if (i == stats_count) {
        if (stats_count == MAX_STATS) {
            fprintf(stderr, "Statistics table full; dropping new process entry\n");
            return 0;
        }
        stats[i].pid = event->pid;
        stats[i].op = event->op;
        stats_count++;
    }

    stats[i].bytes += event->bytes;
    stats[i].operations++;
    memcpy(stats[i].comm, event->comm, sizeof(stats[i].comm));
    memcpy(stats[i].path, event->path, sizeof(stats[i].path));
    stats[i].comm[sizeof(stats[i].comm) - 1] = '\0';
    stats[i].path[sizeof(stats[i].path) - 1] = '\0';
    return 0;
}

static void print_report(void)
{
    size_t i;

    printf("\n%-8s %-7s %-16s %-12s %-10s %s\n",
           "PID", "OP", "COMMAND", "OPERATIONS", "BYTES", "LAST FILE SEEN");
    printf("--------------------------------------------------------------------------------\n");
    for (i = 0; i < stats_count; i++) {
        printf("%-8u %-7s %-16.16s %-12llu %-10llu %s\n",
               stats[i].pid,
               stats[i].op == OP_READ ? "read" : "write",
               stats[i].comm,
               (unsigned long long)stats[i].operations,
               (unsigned long long)stats[i].bytes,
               stats[i].path);
    }
    fflush(stdout);
}

static double monotonic_seconds(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0.0;
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}

int main(void)
{
    struct file_monitor_bpf *skel = NULL;
    struct ring_buffer *ring = NULL;
    int err = 1;
    double next_report;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    skel = file_monitor_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton. Was the project built with make?\n");
        goto cleanup;
    }

    err = file_monitor_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load BPF program (%d). Try running as root and check kernel BTF/BPF support.\n", err);
        goto cleanup;
    }

    err = file_monitor_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF probes (%d). The kernel may not expose vfs_read/vfs_write to kprobes.\n", err);
        goto cleanup;
    }

    ring = ring_buffer__new(bpf_map__fd(skel->maps.events), handle_event, NULL, NULL);
    if (!ring) {
        fprintf(stderr, "Failed to create ring buffer\n");
        err = 1;
        goto cleanup;
    }

    printf("File monitor is running. Reports update every %d seconds; press Ctrl+C to stop.\n",
           REPORT_INTERVAL_SECONDS);
    printf("Note: this reports successful VFS bytes, not physical disk I/O.\n");
    next_report = monotonic_seconds() + REPORT_INTERVAL_SECONDS;

    while (!exiting) {
        double now;
        err = ring_buffer__poll(ring, 250);
        if (err == -EINTR && exiting) {
            err = 0;
            break;
        }
        if (err < 0) {
            fprintf(stderr, "Ring buffer polling failed (%d)\n", err);
            goto cleanup;
        }

        now = monotonic_seconds();
        if (now >= next_report) {
            print_report();
            next_report = now + REPORT_INTERVAL_SECONDS;
        }
    }

    print_report();
    err = 0;

cleanup:
    ring_buffer__free(ring);
    file_monitor_bpf__destroy(skel);
    return err ? 1 : 0;
}
