/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*
 * Soft:        xdp-rss-mon: XDP RSS monitor. Attaches xdp-rss.bpf to a
 *              specified network interface and monitors RSS activity
 *              via trace_pipe.
 *
 * Authors:     Alexandre Cassen, <acassen@gmail.com>
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Affero General Public
 *              License Version 3.0 as published by the Free Software Foundation;
 *              either version 3.0 of the License, or (at your option) any later
 *              version.
 *
 * Copyright (C) 2026 Alexandre Cassen, <acassen@gmail.com>
 */

#define KBUILD_MODNAME "xdp_rss"

#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <linux/bpf.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>

#define GTPU_PORT	2152

struct gtpu_hdr {
	__u8	flags;
	__u8	msg_type;
	__be16	length;
	__be32	teid;
};

/* Returns 1 if GTP-U matched and logged, 0 otherwise. */
static __always_inline int
parse_gtpu(struct xdp_md *ctx, struct udphdr *udp, void *data_end, int ipv6)
{
	__u32 rx_queue = ctx->rx_queue_index;
	struct gtpu_hdr *gtp;

	if (udp->dest != bpf_htons(GTPU_PORT))
		return 0;

	gtp = (void *)(udp + 1);
	if ((void *)(gtp + 1) > data_end)
		return 0;

	bpf_printk("xdp_rss: GTP-U/IPv%c rx_queue=%u teid=0x%08x",
		   ipv6 ? '6' : '4', rx_queue, bpf_ntohl(gtp->teid));
	return 1;
}

static __always_inline int
parse_ipv4(struct xdp_md *ctx, struct ethhdr *eth, void *data_end)
{
	__u32 rx_queue = ctx->rx_queue_index;
	struct udphdr *udp;
	struct iphdr *iph;

	iph = (void *)(eth + 1);
	if ((void *)(iph + 1) > data_end)
		return XDP_ABORTED;

	if (iph->protocol == IPPROTO_UDP) {
		udp = (void *)iph + (iph->ihl << 2);
		if ((void *)(udp + 1) > data_end)
			return XDP_ABORTED;
		if (parse_gtpu(ctx, udp, data_end, 0))
			return XDP_PASS;
	}

	bpf_printk("xdp_rss: IPv4 rx_queue=%u dst=%08x",
		   rx_queue, bpf_ntohl(iph->daddr));
	return XDP_PASS;
}

static __always_inline int
parse_ipv6(struct xdp_md *ctx, struct ethhdr *eth, void *data_end)
{
	__u32 rx_queue = ctx->rx_queue_index;
	struct ipv6hdr *ip6h;
	struct udphdr *udp;
	__u64 dst_hi, dst_lo;

	ip6h = (void *)(eth + 1);
	if ((void *)(ip6h + 1) > data_end)
		return XDP_ABORTED;

	if (ip6h->nexthdr == IPPROTO_UDP) {
		udp = (void *)(ip6h + 1);
		if ((void *)(udp + 1) > data_end)
			return XDP_ABORTED;
		if (parse_gtpu(ctx, udp, data_end, 1))
			return XDP_PASS;
	}

	dst_hi = *(__u64 *)&ip6h->daddr.s6_addr[0];
	dst_lo = *(__u64 *)&ip6h->daddr.s6_addr[8];
	bpf_printk("xdp_rss: IPv6 rx_queue=%u dst=%016llx%016llx",
		   rx_queue, bpf_be64_to_cpu(dst_hi),
		   bpf_be64_to_cpu(dst_lo));
	return XDP_PASS;
}

SEC("xdp")
int xdp_rss(struct xdp_md *ctx)
{
	void *data_end = (void *)(long)ctx->data_end;
	void *data     = (void *)(long)ctx->data;
	struct ethhdr *eth = data;

	if ((void *)(eth + 1) > data_end)
		return XDP_ABORTED;

	if (eth->h_proto == bpf_htons(ETH_P_IP))
		return parse_ipv4(ctx, eth, data_end);

	if (eth->h_proto == bpf_htons(ETH_P_IPV6))
		return parse_ipv6(ctx, eth, data_end);

	return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
