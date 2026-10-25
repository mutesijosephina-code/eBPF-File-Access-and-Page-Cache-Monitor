I’d recommend this as the best intermediate project:

Project title:
eBPF File Access and Page Cache Monitor

Why this is the best choice:
- It fits the Linux Storage, Filesystem, MM & BPF Summit theme very well
- It is advanced enough to look serious, but still feasible for an intermediate developer
- It uses eBPF, which is highly relevant and impressive
- It demonstrates practical Linux systems knowledge instead of only app-level work
- It is doable in a few weeks with a clean scope

Project proposal:

Title:
eBPF File Access and Page Cache Monitor for Linux Storage and Memory Diagnostics

Abstract:
This project will build a lightweight Linux observability tool using eBPF to monitor file access patterns, storage activity, and page cache behavior in real time. The goal is to help developers and system administrators understand which processes are reading and writing files, how often the page cache is being used, and whether memory pressure or filesystem behavior is causing performance bottlenecks. The tool will provide real-time insights into file-level activity, I/O latency, and cache behavior, making it useful for debugging storage and memory issues in Linux systems.

Problem:
Modern Linux workloads often suffer from poor storage performance without obvious root causes. In many systems, readers and writers are not the only contributors to latency—page cache behavior, writeback pressure, and filesystem access patterns can significantly affect throughput and responsiveness. Traditional tools often provide only coarse-grained statistics and do not show how specific files, processes, or memory states interact with storage behavior. There is a clear need for a tool that connects application activity to filesystem and memory behavior in real time.

Objectives:
- Monitor file opens, reads, writes, and close events using eBPF
- Track which processes are generating the most I/O
- Measure basic read/write latency and file activity patterns
- Estimate page cache usage and identify cache hits/misses
- Detect signs of writeback pressure and storage bottlenecks
- Present results in a readable CLI or dashboard format
- Build a useful system for developers, Linux users, and performance analysts

Scope:
The project will focus on:
- eBPF tracing of file operations
- process-level I/O attribution
- page cache and memory-related observations
- user-space collection and summarization
- simple visualization and reporting

Non-goals:
- Full kernel performance tuning
- Advanced distributed tracing
- Deep filesystem-specific optimization
- Full support for every Linux filesystem or every architecture

System design:
1. eBPF programs
- Attach to tracepoints or kprobes related to file operations
- Capture:
  - file opens
  - reads
  - writes
  - fsync
  - page cache access events
  - writeback and dirty-page activity
- Store aggregated metrics in BPF maps

2. Userspace collector
- A Go or Python service will read BPF maps periodically
- Aggregate metrics by:
  - process
  - file path
  - file type
  - filesystem
  - read/write behavior
- Compute summaries such as:
  - top readers/writers
  - hot files
  - read vs write ratios
  - I/O latency trends
  - cache hit estimate

3. Output layer
- Command-line interface for quick debugging
- Optional dashboard using Grafana or a lightweight HTML/Flask interface
- Report tables for:
  - top processes by I/O
  - most active files
  - slowest file operations
  - cache and writeback summary

Key features:
- Process-centric I/O monitoring
- File activity tracking
- Storage and memory correlation
- Top offenders list
- Live reporting
- Lightweight and low-overhead architecture

Proposed implementation details:
- Language: Go or Rust for userspace, C for eBPF programs
- Libraries: libbpf, BCC, or eBPF CO-RE
- Data source: tracepoints and kprobes
- Storage of metrics: BPF maps plus periodic export to user space
- Visualization: CLI output and optional Grafana dashboard

Example output:
- Process: python3
- File: /var/log/app.log
- Reads: 245/s
- Writes: 80/s
- Avg latency: 4.2 ms
- Cache behavior: high page-cache usage
- Writeback pressure: elevated
- Recommendation: investigate file sync overhead and page cache pressure

Why this is relevant:
This project addresses real Linux performance and observability problems. It connects application behavior to storage and memory efficiency, which is exactly the kind of systems work discussed in Linux storage and filesystem communities. It also showcases a practical understanding of eBPF, page cache behavior, and Linux internals—skills highly valued in modern Linux engineering.

Expected outcome:
By the end of the project, the student will have built a working Linux observability tool that:
- demonstrates eBPF tracing fundamentals
- shows understanding of storage and page cache behavior
- provides actionable performance insights
- is valuable for system debugging and optimization

Deliverables:
- Source code for the eBPF monitoring tool
- README with setup instructions
- Sample workloads for testing
- Result summary and screenshots
- Short technical write-up explaining design choices and observations

Suggested timeline:
Weeks 1–2:
- Learn eBPF basics and set up a Linux test environment
- Build a simple file open/read/write tracer

Weeks 3–4:
- Add process aggregation and file-level metrics
- Add read/write latency tracking

Weeks 5–6:
- Add page cache and writeback observations
- Build a user-space collector and summaries

Weeks 7–8:
- Improve output, test on real workloads, document results

Risk and mitigation:
Risk:
- eBPF hooks and kernel compatibility issues
Mitigation:
- Use tracepoints with stable interfaces and keep the scope focused on a modern Linux distribution

Risk:
- Too much complexity in page cache analysis
Mitigation:
- Start with basic file activity and cache estimation, then extend gradually

Risk:
- Limited time for dashboards
Mitigation:
- Provide CLI output first and add visualization only if time allows

Final recommendation:
This is the ideal intermediate-level project because it is practical, visible, technically strong, and aligned with the goals of the Linux Storage, Filesystem, MM & BPF Summit. It shows that you understand the Linux kernel, filesystem behavior, memory management, and modern tracing tools.

If you want, I can now do one of the following:
- write a shorter 1-page version of this proposal
- turn this into a GitHub-ready README
- give you the exact project architecture diagram in text form
- help you choose the best stack: Go + libbpf or Rust + libbpf
- write a more polished “conference-style” proposal for submission
