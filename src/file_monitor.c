#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "file_monitor.h"

static volatile sig_atomic_t stop = 0;

static void handle_signal(int sig)
{
    (void)sig;
    stop = 1;
}

int main(void)
{
    struct bpf_object *obj;
    int err;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Loading eBPF file monitor...\n");

    obj = bpf_object__open_file("src/file_monitor.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }

    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        return 1;
    }

    printf("Monitor started. Press Ctrl+C to stop.\n");

    while (!stop) {
        sleep(1);
    }

    printf("Stopping monitor.\n");
    bpf_object__close(obj);
    return 0;
}
