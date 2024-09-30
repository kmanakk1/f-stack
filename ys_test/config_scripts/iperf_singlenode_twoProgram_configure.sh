#!/bin/bash

TEST_IFACE=(enp4s0f0np0 enp132s0f0np0)
ETH1_DEV="${TEST_IFACE[0]}"
ETH2_DEV="${TEST_IFACE[1]}"
IFACE_IP=(192.168.100.1 192.168.101.1)
IFACE_FAKE_IP=(192.168.102.1 192.168.103.1)

eth1_mac=$(ifconfig "$ETH1_DEV"|grep -i "ether" |awk '{print $2}')
eth2_mac=$(ifconfig "$ETH2_DEV"|grep -i "ether" |awk '{print $2}')
eth1_ip="${IFACE_IP[0]}"
eth2_ip="${IFACE_IP[1]}"
eth1_fake_ip="${IFACE_FAKE_IP[0]}"
eth2_fake_ip="${IFACE_FAKE_IP[1]}"
ifconfig "$ETH1_DEV" "$eth1_ip/24" up
ifconfig "$ETH2_DEV" "$eth2_ip/24" up

ip -s -s neigh flush all
iptables -t nat -F

iptables -t nat -A POSTROUTING -s "$eth1_ip" -d "$eth2_fake_ip" -j SNAT --to-source "$eth1_fake_ip"
iptables -t nat -A PREROUTING -d "$eth1_fake_ip" -j DNAT --to-destination "$eth1_ip"

iptables -t nat -A POSTROUTING -s "$eth2_ip" -d "$eth1_fake_ip" -j SNAT --to-source "$eth2_fake_ip"
iptables -t nat -A PREROUTING -d "$eth2_fake_ip" -j DNAT --to-destination "$eth2_ip"

ip route add "$eth2_fake_ip" dev "$ETH1_DEV"
arp -i "$ETH1_DEV" -s "$eth2_fake_ip" "$eth2_mac"

ip route add "$eth1_fake_ip" dev "$ETH2_DEV"
arp -i "$ETH2_DEV" -s "$eth1_fake_ip" "$eth1_mac"

ip route add 192.168.182.3 dev enp132s0f0np0
arp -i enp132s0f0np0 -s 192.168.182.5 08:c0:eb:aa:37:b8
arp -i enp132s0f0np0 -s 192.168.182.3 08:c0:eb:aa:37:b8
arp -i enp4s0f0np0 -s 192.168.182.4 08:c0:eb:d1:f8:f2