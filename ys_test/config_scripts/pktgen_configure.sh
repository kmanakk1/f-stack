#!/bin/bash

# 清除现有配置
# modprobe pktgen
# PGDEV=/proc/net/pktgen/kpktgend_1
# echo "rem_device_all" > $PGDEV
# echo "add_device enp132s0f0np0" > $PGDEV

# 配置流量生成参数
PGDEV=/proc/net/pktgen/enp132s0f0np0
echo "count 1000000000" > $PGDEV  # 生成数据包的数量
echo "clone_skb 0" > $PGDEV    # 0表示不克隆SKB
echo "frags 0" > $PGDEV
echo "pkt_size 100" > $PGDEV    # 数据包大小
echo "delay 0" > $PGDEV        # 延迟，0表示尽可能快
echo "dst 192.168.182.5" > $PGDEV # 目标IP地址
echo "src_min 192.168.182.6" > $PGDEV
echo "src_max 192.168.182.6" > $PGDEV
echo "udp_src_min 9" > $PGDEV
echo "udp_src_max 9" > $PGDEV
echo "dst_mac 08:c0:eb:aa:37:b8" > $PGDEV # 目标MAC地址
echo "udp_dst_min 8080" > $PGDEV # 目标UDP端口（最小值）
echo "udp_dst_max 8080" > $PGDEV # 目标UDP端口（最大值）

# 启动流量生成
#PGDEV=/proc/net/pktgen/pgctrl
#echo "start" > $PGDEV
#echo "Traffic generation started."