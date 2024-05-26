#!/bin/sh

[ "$2" == "" ] && echo "Usage: $0 <ethif> <tapif>"
[ "$2" == "" ] && exit

ETH=$1
TAP=$2
echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -A POSTROUTING -o ${ETH} -j MASQUERADE
iptables -A FORWARD -i ${ETH} -o ${TAP} -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i ${TAP} -o ${ETH} -j ACCEPT
