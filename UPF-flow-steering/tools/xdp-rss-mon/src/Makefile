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

CC	 ?= gcc
INCLUDES = -Iinclude $(LIBBPF_CFLAGS) $(LIBXDP_CFLAGS)
CFLAGS	 = -g -O2 -ggdb $(INCLUDES) -Werror -Wall -Wunused -Wstrict-prototypes -D_GNU_SOURCE -MMD
#DEFS	 = -D_DEBUG_
SUBDIRS   =
DEFS	 =
COMPILE	 = $(CC) $(CFLAGS) $(DEFS)

%.o: %.c
	@echo "  CC" $@
	@$(COMPILE) -c $<

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
all: $(OBJS)
	@set -e; \
	for i in $(SUBDIRS); do \
	$(MAKE) -C $$i || exit 1; done

clean:
	@set -e; \
	for i in $(SUBDIRS); do \
	$(MAKE) -C $$i clean || exit 1; done
	@rm -vf *.a *.o *.d *~

-include *.d
