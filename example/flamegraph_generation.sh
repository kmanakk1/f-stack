#!/bin/bash


perf_data="./perf.data"
flamegraph="../../FlameGraph"

# check perf.data existence
if [ -e "${perf_data}" ]; then
    echo "Path \"./perf.data\" exists"
else
    echo "Path \"./perf.data\" nonexists, please set perf.data in this dir"
    exit 1
fi

if [ "$(command -v perf)" ]; then
    echo "perf exists on system"
else
    echo "perf nonexists on system, please install perf" 
    exit 1
fi

if [ -e "${flamegraph}" ]; then
    echo "${flamegraph} exists"
else
    echo "${flamegraph} nonexists, please set flamegraph dir in ../.."
    exit 1
fi


perf script -i perf.data &> perf.unfold
../../FlameGraph/stackcollapse-perf.pl perf.unfold &> perf.folded
../../FlameGraph/flamegraph.pl perf.folded > perf.svg

rm ./perf.unfold ./perf.folded 

echo "successful to generate flamegraph"
exit 0