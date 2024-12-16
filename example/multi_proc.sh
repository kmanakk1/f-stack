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
for i in {0..3}
do   
   # 启动程序并传递 -p 参数（程序号和端口号），放到后台运行
   ./helloworld -p "$i" &
   pids+=($!)
done

wait

