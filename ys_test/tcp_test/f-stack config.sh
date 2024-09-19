

TEST_IFACE=(enp4s0f0np0 enp132s0f0np0)
ETH1_DEV="${TEST_IFACE[0]}"
ETH2_DEV="${TEST_IFACE[1]}"
IFACE_IP=(192.168.182.3 192.168.182.4)
IFACE_FAKE_IP=(192.168.102.1 192.168.103.1)


eth1_ip="${IFACE_IP[0]}"
eth2_ip="${IFACE_IP[1]}"
ifconfig "$ETH1_DEV" "$eth1_ip/24" up
ifconfig "$ETH2_DEV" "$eth2_ip/24" up
