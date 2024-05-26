
In one terminal, start testpmd:
```
dpdk-testpmd --vdev=net_tap0,iface=dtap0 --vdev=net_tap1,iface=dtap1 -- --tx-ip=10.1.1.1,10.1.1.2 -i
```
In another terminal start socat to connect the taps together:
```
socat interface:dtap0 interface:dtap1
```

Then finally at the testpmd> prompt in the first terminal:
Create a flow for destination 10.1.1.2 where we add to queue 0
then start transmitting packets
```
flow create 0 priority 1 ingress pattern eth / ipv4 dst is 10.1.1.2 / end actions queue index 0 / end
start tx_first
show port stats 0
stop
```

Optional: observe transmitted packets
```
tcpdump -i dtap0
```