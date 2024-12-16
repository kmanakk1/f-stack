#!/bin/bash

pids=()

cleanup()
{
    for pid in "${pids[@]}"; do
        kill "$pid"
    done
    exit 1
}


trap cleanup SIGINT

# 启动10个并行程序
for _ in {1..10}
do
   ../tcp_client_performance &  # 将程序放在后台运行
   pids+=($!)
done

wait

