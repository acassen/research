/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*
 * Soft:        Split a GTP-U TEID range over N queues (N MUST be a power of 2).
 *              Usage: ./teid-split [-v] <num-queues> [<start> <size>]
 *
 *              Without start/size the full 32-bit TEID space is split.
 *              start and size accept decimal or 0x hex.
 *              size must be a power of 2 and large enough for N queues.
 *
 *              Example: 8 queues -> mask 0xe0000000, 536870912 TEIDs/queue
 *                       8 queues 0x10000000 0x800000 -> 8 x 524288 TEIDs from 0x10000000
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
#include <stdint.h>
#include <unistd.h>

static int
split(int num_queues, uint64_t start, uint64_t size, int verbose)
{
	uint64_t count, mask;
	int bits, total_bits, shift, q;

	if (num_queues <= 0 || (num_queues & (num_queues - 1))) {
		fprintf(stderr, "Error: num-queues must be a power of 2\n");
		return 1;
	}

	if (size & (size - 1)) {
		fprintf(stderr, "Error: size must be a power of 2\n");
		return 1;
	}

	if ((uint64_t)size < (uint64_t)num_queues) {
		fprintf(stderr, "Error: size %lu too small for %d queues\n",
			size, num_queues);
		return 1;
	}

	if (start + size > (uint64_t)1 << 32) {
		fprintf(stderr, "Error: range exceeds 32-bit TEID space\n");
		return 1;
	}

	count = size / num_queues;

	for (bits = 0; (1 << bits) < num_queues; bits++);
	for (total_bits = 0; ((uint64_t)1 << total_bits) < size; total_bits++);

	shift = total_bits - bits;
	mask = (~(size - 1) & 0xffffffff) | (((uint64_t)(1 << bits) - 1) << shift);
	mask &= 0xffffffff;

	for (q = 0; q < num_queues; q++) {
		uint64_t base = start + (uint64_t)q * count;
		if (verbose)
			printf("q%-3d 0x%08lx/0x%08lx  (%lu TEIDs)\n",
			       q, base, mask, count);
		else
			printf("q%-3d 0x%08lx/0x%08lx\n", q, base, mask);
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	uint64_t start, size;
	int num_queues, verbose = 0, opt;

	while ((opt = getopt(argc, argv, "v")) != -1) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		default:
			fprintf(stderr, "Usage: %s [-v] <num-queues> [<start> <size>]\n",
				argv[0]);
			return 1;
		}
	}

	if (argc - optind != 1 && argc - optind != 3) {
		fprintf(stderr, "Usage: %s [-v] <num-queues> [<start> <size>]\n",
			argv[0]);
		return 1;
	}

	num_queues = (int)strtol(argv[optind], NULL, 0);

	if (argc - optind == 3) {
		start = strtoull(argv[optind + 1], NULL, 0);
		size  = strtoull(argv[optind + 2], NULL, 0);
	} else {
		start = 0;
		size  = (uint64_t)1 << 32;
	}

	return split(num_queues, start, size, verbose);
}
