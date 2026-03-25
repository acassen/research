# UPF Flow Steering

Companion tools and kernel patches for debugging and testing GTP-U TEID range
partitioning across NIC receive queues, to be incorporated into GTP-Guard.

The goal is to efficiently load balance GTP-U tunnel traffic at UPF side
by steering packets based on TEID ranges or IP prefixes to dedicated receive
queues.

## Repository Layout

```
.
├── tools/          userspace utilities and test scripts
├── take1/          kernel patch: GTP-U RSS via hardware hash
├── take2/          kernel patch: ethtool NFC rule support
├── take3/          kernel patch: TC offload with rx_queue action
└── take4/          kernel patch: iproute2 arbitrary mask support
```

## Tools

### ip-pfx-split

Splits an IPv4 or IPv6 prefix evenly across N receive queues (N must be a
power of 2). Uses the log2(N) bits immediately after the prefix length as
queue selectors and emits one sub-prefix per queue.

```
./ip-pfx-split [-v] <prefix/len> <num-queues>

Example:
  ./ip-pfx-split 10.0.0.0/8 8
  ./ip-pfx-split 2001:db8::/46 8
```

### teid-split

Splits a 32-bit GTP-U TEID space (or a sub-range) across N receive queues.
Outputs a base TEID and bitmask per queue suitable for use as TC flower filter
parameters.

```
./teid-split <num-queues> [start size]

Example:
  ./teid-split 8
  ./teid-split 8 0x10000000 0x800000
```

### pkt-send.py

Python3/Scapy packet generator for sending GTP-U or plain UDP packets over a
specified interface. Used to exercise and validate flow steering rules.

```
./pkt-send.py -i IFACE -s SRC_MAC -d DST_MAC -S SRC_IP -D DST_IP [-t TEID] [-n COUNT]
```

### xdp-rss-mon

XDP-based monitoring tool that attaches an eBPF program to a NIC and reports,
via the kernel trace pipe, which receive queue each incoming GTP-U packet lands
on, along with the TEID or destination IP address. Useful for verifying that
steering rules produce the expected queue distribution.

```
xdp-rss-mon/
├── src/main.c          loader: attaches XDP program, drains trace_pipe
└── src/bpf/xdp-rss.c  eBPF/XDP program: parses GTP-U, logs queue + TEID
```

Build with `make` inside `xdp-rss-mon/`. Requires libbpf and a kernel with
XDP driver-mode support.

### setup-fs.sh

Configures TC flower ingress filters on interface `p0` (VLAN 522) to steer:
- GTP-U packets by TEID range (IPv4 and IPv6 outer)
- Plain IP packets by destination prefix

Calls `teid-split` and `ip-pfx-split` internally to derive the per-queue
ranges, then programs one `skbedit queue_mapping` action per queue.

### test-fs.sh

Sends one test packet per queue for each steering category (GTP-U/IPv4,
GTP-U/IPv6, IPv4 dst, IPv6 dst) through `pkt-send.py` to validate the
rules installed by `setup-fs.sh`.

### pin-mlx-irqs.sh

Pins NIC MSI-X interrupts to a specified set of CPU cores using
round-robin assignment. Reduces latency and improves cache locality when
each queue is handled by a dedicated core.

```
./pin-mlx-irqs.sh <PCI_DEVICE> <CPU_LIST>

Example:
  ./pin-mlx-irqs.sh 0000:31:00.0 2-23
```

## Kernel Patch Iterations

Each `take` directory contains patches against the Linux kernel and/or
iproute2, representing successive design iterations.

| Dir    | Description |
|--------|-------------|
| take1  | Add `MLX5_TT_IPV4/6_GTPU` traffic types and `GTPU_TEID` hash field; expose GTPU flows via ethtool rxfh |
| take2  | Add `GTPU_V4/V6_FLOW` to ethtool NFC so users can steer GTP-U via `ethtool -N` with TEID masks |
| take3  | TC offload for `FLOW_ACTION_RX_QUEUE_MAPPING`; GTP-U TEID range matching in TC flower; iproute2 `gtp_opts` support |
| take4  | Refine take3; add arbitrary mask support for `enc_key_id` and IP addresses in iproute2 flower |

Enjoy,
Alexandre