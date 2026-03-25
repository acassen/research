#!/usr/bin/env bash
# test-fs.sh - Send one packet per queue range to validate flow steering rules.
#              Covers GTP-U over IPv4, GTP-U over IPv6, plain IPv4 and plain IPv6.

IFACE=p0
SRC_MAC=aa:bb:cc:dd:ee:01
DST_MAC=aa:bb:cc:dd:ee:02

# Outer IPs for GTP-U over IPv4
GTP4_SRC=192.168.1.1
GTP4_DST=192.168.1.2

# Outer IPs for GTP-U over IPv6
GTP6_SRC=fd00::1
GTP6_DST=fd00::2

# Source IPs for plain L3 tests
V4_SRC=172.16.0.1
V6_SRC=fd00::1

QUEUES=8
V4_PFX=10.0.0.0/8
V6_PFX=2001:db8::/46

echo "=== GTP-U over IPv4 TEID steering ==="
./teid-split ${QUEUES} | while read qname teid_range; do
	q=${qname#q}
	base=$(( ${teid_range%%/*} ))
	# TEID 0 is reserved; use base+1 for queue 0
	teid=$(printf '0x%08x' $(( base == 0 ? base + 1 : base )))
	./pkt-send.py -i ${IFACE} -s ${SRC_MAC} -d ${DST_MAC} \
		      -S ${GTP4_SRC} -D ${GTP4_DST} -t ${teid}
done

echo ""
echo "=== GTP-U over IPv6 TEID steering ==="
./teid-split ${QUEUES} | while read qname teid_range; do
	q=${qname#q}
	base=$(( ${teid_range%%/*} ))
	teid=$(printf '0x%08x' $(( base == 0 ? base + 1 : base )))
	./pkt-send.py -i ${IFACE} -s ${SRC_MAC} -d ${DST_MAC} \
		      -S ${GTP6_SRC} -D ${GTP6_DST} -t ${teid}
done

echo ""
echo "=== IPv4 destination steering ==="
./ip-pfx-split ${V4_PFX} ${QUEUES} | while read qname pfx; do
	q=${qname#q}
	addr=${pfx%%/*}
	./pkt-send.py -i ${IFACE} -s ${SRC_MAC} -d ${DST_MAC} \
		      -S ${V4_SRC} -D ${addr}
done

echo ""
echo "=== IPv6 destination steering ==="
./ip-pfx-split ${V6_PFX} ${QUEUES} | while read qname pfx; do
	q=${qname#q}
	addr=${pfx%%/*}
	./pkt-send.py -i ${IFACE} -s ${SRC_MAC} -d ${DST_MAC} \
		      -S ${V6_SRC} -D ${addr}
done
