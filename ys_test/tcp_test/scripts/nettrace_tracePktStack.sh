#!/bin/bash

if [ "$(command -v nettrace)" ]; then
    echo "nettrace exists"
else
    echo "nettrace nonexists" 
    exit 1
fi

nettrace --pkt-len 1500 -p tcp -d 192.168.182.4 