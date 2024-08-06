#!/bin/sh
ip link delete vethp0
ip link delete vethp1
ip netns delete ns1