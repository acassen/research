# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Soft:        RX Queue RSS monitoring via XDP
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
#

EXEC       = xdp-rss-mon
BIN        = bin

prefix ?= /usr/local
exec_prefix ?= ${prefix}
sbindir     ?= ${exec_prefix}/sbin
sysconfdir  ?= ${prefix}/etc

CC        ?= gcc
CLANG     ?= clang
LLC       ?= llc
OBJCOPY   ?= objcopy
LDFLAGS   = -lpthread -lcrypt -ggdb -lm -lz -lresolv -lelf
SUBDIRS   = src src/bpf

LIBBPF_CFLAGS := $(shell pkg-config --cflags libbpf 2>/dev/null)
LIBBPF_LIBS   := $(shell pkg-config --libs   libbpf 2>/dev/null || echo -lbpf)

export LIBBPF_CFLAGS

all:
	@set -e; \
	for i in $(SUBDIRS); do \
	$(MAKE) -C $$i || exit 1; done && \
	echo "Building $(BIN)/$(EXEC)" && \
	$(CC) -o $(BIN)/$(EXEC) src/main.o $(LIBBPF_LIBS) $(LDFLAGS) && \
	echo "Building $(BIN)/$(EXEC)"
	@echo ""
	@echo "Make complete"

clean:
	@set -e; \
	for i in $(SUBDIRS); do \
	$(MAKE) -C $$i clean; done
	@rm -vf $(BIN)/$(EXEC)
	@echo ""
	@echo "Make complete"
