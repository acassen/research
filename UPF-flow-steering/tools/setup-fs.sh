./tc qdisc add dev p0 clsact

# GTP-U Steering
#
# ./teid-split 8
# q0   0x00000000/0xe0000000
# q1   0x20000000/0xe0000000
# q2   0x40000000/0xe0000000
# q3   0x60000000/0xe0000000
# q4   0x80000000/0xe0000000
# q5   0xa0000000/0xe0000000
# q6   0xc0000000/0xe0000000
# q7   0xe0000000/0xe0000000

./teid-split 8 | while read qname teid; do
#	./tc filter add dev p0 ingress protocol ip flower skip_sw	\
#			enc_dst_port 2152 enc_key_id ${teid}		\
#			action skbedit queue_mapping $q skip_sw

	# over IPv4
	./tc filter add dev p0 ingress protocol 802.1q flower skip_sw	\
			vlan_id 522 vlan_ethtype ip			\
			enc_dst_port 2152 enc_key_id ${teid}		\
			action skbedit queue_mapping ${qname#q} skip_sw

	# over IPv6
	./tc filter add dev p0 ingress protocol 802.1q flower skip_sw	\
			vlan_id 522 vlan_ethtype ipv6			\
			enc_dst_port 2152 enc_key_id ${teid}		\
			action skbedit queue_mapping ${qname#q} skip_sw
done


# IPv6 Steering
#
# IPv6 prefix 2001:db8::/46 split over 8 queues:
#
# ./ip-pfx-split 2001:db8::/46 8
# q0   2001:db8::/ffff:ffff:ffff:8000::
# q1   2001:db8:0:8000::/ffff:ffff:ffff:8000::
# q2   2001:db8:1::/ffff:ffff:ffff:8000::
# q3   2001:db8:1:8000::/ffff:ffff:ffff:8000::
# q4   2001:db8:2::/ffff:ffff:ffff:8000::
# q5   2001:db8:2:8000::/ffff:ffff:ffff:8000::
# q6   2001:db8:3::/ffff:ffff:ffff:8000::
# q7   2001:db8:3:8000::/ffff:ffff:ffff:8000::

./ip-pfx-split 2001:db8::/46 8 | while read qname pfx; do
#	./tc filter add dev p0 ingress protocol ipv6 flower skip_sw	\
#			dst_ip ${pfx}				\
#			action skbedit queue_mapping $q skip_sw

	./tc filter add dev p0 ingress protocol 802.1q flower skip_sw	\
			vlan_id 522 vlan_ethtype ipv6			\
			dst_ip ${pfx}					\
			action skbedit queue_mapping ${qname#q} skip_sw
done


# IPv4 Steering
#
# IPv4 prefix 10.0.0.0/8 split over 8 queues:
#
# ./ip-pfx-split 10.0.0.0/8 8
# q0   10.0.0.0/255.224.0.0
# q1   10.32.0.0/255.224.0.0
# q2   10.64.0.0/255.224.0.0
# q3   10.96.0.0/255.224.0.0
# q4   10.128.0.0/255.224.0.0
# q5   10.160.0.0/255.224.0.0
# q6   10.192.0.0/255.224.0.0
# q7   10.224.0.0/255.224.0.0

./ip-pfx-split 10.0.0.0/8 8 | while read qname pfx; do
#	./tc filter add dev p0 ingress protocol ip flower skip_sw	\
#			dst_ip ${pfx}				\
#			action skbedit queue_mapping $q skip_sw

	./tc filter add dev p0 ingress protocol 802.1q flower skip_sw	\
			vlan_id 522 vlan_ethtype ip			\
			dst_ip ${pfx}					\
			action skbedit queue_mapping ${qname#q} skip_sw
done

./tc filter show dev p0 ingress
