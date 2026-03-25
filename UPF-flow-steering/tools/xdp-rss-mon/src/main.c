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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <bpf/libbpf.h>

#define TRACE_PIPE_PATH		"/sys/kernel/debug/tracing/trace_pipe"
#define TRACE_BUF_SIZE		4096

#define BPF_PROG_DEFAULT_PATH	"./xdp-rss.bpf"
#define BPF_PROG_NAME		"xdp_rss"

static volatile int running = 1;

static void
sig_handler(int sig)
{
	running = 0;
}

static void
usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"  -f path    path to xdp-rss.bpf object (default: %s)\n"
		"  -i iface   network interface to attach XDP program to\n",
		prog, BPF_PROG_DEFAULT_PATH);
}

/*
 * xdp_load_and_attach
 */
static int
xdp_load_and_attach(const char *bpf_path, int ifindex, __u32 xdp_flags)
{
	struct bpf_object *obj;
	struct bpf_program *prog;
	int prog_fd, err;

	obj = bpf_object__open(bpf_path);
	if (!obj) {
		fprintf(stderr, "Error: failed to open BPF object '%s': %s\n",
			bpf_path, strerror(errno));
		return -errno;
	}

	err = bpf_object__load(obj);
	if (err) {
		fprintf(stderr, "Error: failed to load BPF object: %s\n",
			strerror(-err));
		goto out;
	}

	prog = bpf_object__find_program_by_name(obj, BPF_PROG_NAME);
	if (!prog) {
		fprintf(stderr, "Error: BPF program '%s' not found in '%s'\n",
			BPF_PROG_NAME, bpf_path);
		err = -ENOENT;
		goto out;
	}

	prog_fd = bpf_program__fd(prog);

	err = bpf_xdp_attach(ifindex, prog_fd, xdp_flags, NULL);
	if (err)
		fprintf(stderr, "Error: failed to attach XDP program: %s\n",
			strerror(-err));
out:
	bpf_object__close(obj);
	return err;
}

/*
 * trace_pipe_open - open trace_pipe non-blocking, warn if unavailable
 */
static int
trace_pipe_open(void)
{
	int fd;

	fd = open(TRACE_PIPE_PATH, O_RDONLY | O_NONBLOCK);
	if (fd < 0)
		fprintf(stderr, "Warning: cannot open trace_pipe: %s\n",
			strerror(errno));
	return fd;
}

static void
trace_pipe_drain(int fd)
{
	char buf[TRACE_BUF_SIZE];
	ssize_t n;

	while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
		buf[n] = '\0';
		fputs(buf, stdout);
	}
}

/*
 * main
 */
int
main(int argc, char *argv[])
{
	const char *bpf_path = BPF_PROG_DEFAULT_PATH;
	const char *ifname = NULL;
	__u32 xdp_flags = XDP_FLAGS_DRV_MODE | XDP_FLAGS_UPDATE_IF_NOEXIST;
	int ifindex = 0, trace_fd = -1, err = 0, opt;
	struct pollfd pfds[1];

	while ((opt = getopt(argc, argv, "f:i:h")) != -1) {
		switch (opt) {
		case 'f':
			bpf_path = optarg;
			break;
		case 'i':
			ifname = optarg;
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (!ifname) {
		fprintf(stderr, "Error: -i iface required\n");
		usage(argv[0]);
		return 1;
	}

	ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		fprintf(stderr, "Error: interface '%s' not found: %s\n",
			ifname, strerror(errno));
		return 1;
	}

	err = xdp_load_and_attach(bpf_path, ifindex, xdp_flags);
	if (err)
		return 1;

	printf("XDP program '%s' attached to interface '%s' (ifindex %d)\n",
	       BPF_PROG_NAME, ifname, ifindex);

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	trace_fd = trace_pipe_open();

	printf("RSS activity monitoring... (Ctrl-C to stop)\n");

	pfds[0].fd = trace_fd;
	pfds[0].events = POLLIN;

	while (running) {
		if (poll(pfds, trace_fd >= 0 ? 1 : 0, -1) < 0) {
			if (errno == EINTR)
				break;
			fprintf(stderr, "Error: poll: %s\n", strerror(errno));
			err = -errno;
			goto out;
		}

		if (trace_fd >= 0 && (pfds[0].revents & POLLIN))
			trace_pipe_drain(trace_fd);
	}

	err = 0;
	printf("\nDetaching XDP program from '%s'...\n", ifname);
	bpf_xdp_detach(ifindex, xdp_flags, NULL);
out:
	if (trace_fd >= 0)
		close(trace_fd);
	return err;
}
