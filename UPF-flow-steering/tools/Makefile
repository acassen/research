# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Soft:        Build tools
#
# Authors:     Alexandre Cassen, <acassen@gmail.com>
#
#              This program is free software; you can redistribute it and/or
#              modify it under the terms of the GNU Affero General Public
#              License Version 3.0 as published by the Free Software Foundation;
#              either version 3.0 of the License, or (at your option) any later
#              version.
#
# Copyright (C) 2026 Alexandre Cassen, <acassen@gmail.com>

CC	= gcc
CFLAGS	= -Wall -Wextra -O2

PROGS	= ip-pfx-split teid-split pkt-send

all: $(PROGS)

ip-pfx-split: ip-pfx-split.c
	$(CC) $(CFLAGS) -o $@ $<

teid-split: teid-split.c
	$(CC) $(CFLAGS) -o $@ $<

pkt-send: pkt-send.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS)

.PHONY: all clean
