#!/bin/bash


tcpdump -i enp132s0f0np0 -w ../capture1.pcap &  # 后台运行 tcpdump

sleep 0.1                  # 等待 30 毫秒（30000 微秒）

kill $!                            # 终止 tcpdump
