#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Soft:        GTP-U/UDP packet generator using Scapy
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

import argparse
import sys
import time
from scapy.all import Ether, IP, IPv6, UDP, Raw, sendp
from scapy.contrib.gtp import GTP_U_Header

GTP_U_PORT = 2152
UDP_PORT   = 1234
PAYLOAD    = b'\x8b\xad\xf0\x0d'

def build_packet(src_mac, dst_mac, src_ip, dst_ip, teid):
    is_v6 = ':' in src_ip
    l3 = IPv6(src=src_ip, dst=dst_ip) if is_v6 else IP(src=src_ip, dst=dst_ip)
    port = GTP_U_PORT if teid else UDP_PORT
    udp = UDP(sport=port, dport=port)
    if teid:
        inner = GTP_U_Header(teid=teid) / Raw(PAYLOAD)
    else:
        inner = Raw(PAYLOAD)
    return Ether(src=src_mac, dst=dst_mac) / l3 / udp / inner

def main():
    p = argparse.ArgumentParser(
        description='GTP-U/UDP packet generator')
    p.add_argument('-i', metavar='IFACE',   required=True,  help='Output network interface')
    p.add_argument('-s', metavar='SRC_MAC', required=True,  help='Source MAC (xx:xx:xx:xx:xx:xx)')
    p.add_argument('-d', metavar='DST_MAC', required=True,  help='Destination MAC (xx:xx:xx:xx:xx:xx)')
    p.add_argument('-S', metavar='SRC_IP',  required=True,  help='Source IPv4 or IPv6 address')
    p.add_argument('-D', metavar='DST_IP',  required=True,  help='Destination IPv4 or IPv6 address')
    p.add_argument('-t', metavar='TEID',    default=0,      help='GTP-U TEID (decimal or 0x hex); omit for plain UDP', type=lambda x: int(x, 0))
    p.add_argument('-n', metavar='COUNT',   default=1,      help='Number of packets to send (default: 1)', type=int)
    args = p.parse_args()

    if args.n < 1:
        p.error('-n must be > 0')

    if (':' in args.S) != (':' in args.D):
        p.error('src and dst must be the same address family')

    pkt = build_packet(args.s, args.d, args.S, args.D, args.t)

    if args.t:
        print(f'Sending {args.n} GTP-U packet(s): TEID=0x{args.t:08x}  {args.i}  IPv{"6" if ":" in args.S else "4"}')
    else:
        print(f'Sending {args.n} UDP packet(s): {args.i}  IPv{"6" if ":" in args.S else "4"}')

    for _ in range(args.n):
        sendp(pkt, iface=args.i, verbose=False)
        if _ < args.n - 1:
            time.sleep(1)

if __name__ == '__main__':
    main()
