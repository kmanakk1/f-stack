#!/bin/sh
# netns, create veth pairs
ip link add vethp0 type veth peer name vethp1       # create veth pair
ip netns add ns1                                    # create namespace
ip link set dev vethp0 netns ns1                    # put pair end 0 in namespace ns1
ip netns exec ns1 ifconfig vethp0 10.1.1.5/24       # set pair end 0 ip address

# do hugepage stuff
echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mkdir -p /mnt/huge
mount -t hugetlbfs nodev /mnt/huge

./example/helloworld -c ./example/config.ini