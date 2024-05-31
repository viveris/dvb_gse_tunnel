#!/bin/bash

# Copyright 2023, Viveris Technologies
# Distributed under the terms of the MIT License

# Binaries directories:
#   c -----------------> c/src/tap_udp/
#   rust --------------> rust/target/release/
#   rust_testimonial --> rust_testimonial/target/release/

setup_encap_tx() {
	local TUN_IP=$1
	local TUN_MAC=$2
	local NET_IP=$3
	local TGT_IP=$4
	local GW_IP=$5
	local GW_MAC=$6
	local PREFIX=""
	[ $# -eq 7 ] && PREFIX="ip netns exec $7 "

	local TAP_IFACE="tap0"
	local BR_TAP_IFACE="br0"
	local NET_IFACE="iface"

	local GW_IFACE=${BR_TAP_IFACE}

	# Create encap TX TAP interface
	${PREFIX}ip tuntap add mode tap ${TAP_IFACE}
	${PREFIX}ip link set dev ${TAP_IFACE} up

	# Create bridge to set an IPv4 address to the encap TX TAP interface
	${PREFIX}ip link add name ${BR_TAP_IFACE} type bridge
	${PREFIX}ip link set dev ${BR_TAP_IFACE} address ${TUN_MAC}
	${PREFIX}ip link set dev ${TAP_IFACE} master ${BR_TAP_IFACE}
	${PREFIX}ip addr add ${TUN_IP}/24 dev ${BR_TAP_IFACE}
	${PREFIX}ip link set dev ${BR_TAP_IFACE} up

	# Create network interface
	${PREFIX}ip link add ${NET_IFACE} type dummy
	${PREFIX}ip addr add ${NET_IP}/24 dev ${NET_IFACE}
	${PREFIX}ip link set dev ${NET_IFACE} up

	# Add route and neighbor
	${PREFIX}ip neighbor add ${GW_IP} lladdr ${GW_MAC} dev ${GW_IFACE}
	${PREFIX}ip route add ${TGT_IP}/32 via ${GW_IP}
}

setup_encap_rx() {
	local TUN_IP=$1
	local TUN_MAC=$2
	local NET_IP=$3
	local TGT_IP=$4
	local GW_IFACE=$5
	local GW_IP=$6
	local GW_MAC=$7
	local PREFIX=""
	[ $# -eq 8 ] && PREFIX="ip netns exec $8 "

	local TAP_IFACE="tap0"
	local BR_TAP_IFACE="br0"
	local NET_IFACE="iface"

	# Create encap RX TAP interface
	${PREFIX}ip tuntap add mode tap ${TAP_IFACE}
	${PREFIX}ip link set dev ${TAP_IFACE} up

	# Create bridge to set an IPv4 address to the encap RX TAP interface
	${PREFIX}ip link add name ${BR_TAP_IFACE} type bridge
	${PREFIX}ip link set dev ${BR_TAP_IFACE} address ${TUN_MAC}
	${PREFIX}ip link set dev ${TAP_IFACE} master ${BR_TAP_IFACE}
	${PREFIX}ip addr add ${TUN_IP}/24 dev ${BR_TAP_IFACE}
	${PREFIX}ip link set dev ${BR_TAP_IFACE} up

	# Create network interface
	${PREFIX}ip link add ${NET_IFACE} type dummy
	${PREFIX}ip addr add ${NET_IP}/24 dev ${NET_IFACE}
	${PREFIX}ip link set dev ${NET_IFACE} up

	# Add route and neighbor
	${PREFIX}ip neighbor add ${GW_IP} lladdr ${GW_MAC} dev ${GW_IFACE}
	${PREFIX}ip route add ${TGT_IP}/32 via ${GW_IP}
}

cleanup_encap_tx() {
	local PREFIX=""
	[ $# -eq 1 ] && PREFIX="ip netns exec $1 "

	local TAP_IFACE="tap0"
	local BR_TAP_IFACE="br0"
	local NET_IFACE="iface"
	
	${PREFIX}ip link del ${NET_IFACE}
	${PREFIX}ip link del ${BR_TAP_IFACE}
	${PREFIX}ip link del ${TAP_IFACE}
}

cleanup_encap_rx() {
	local PREFIX=""
	[ $# -eq 1 ] && PREFIX="ip netns exec $1 "

	local TAP_IFACE="tap0"
	local BR_TAP_IFACE="br0"
	local NET_IFACE="iface"
	
	${PREFIX}ip link del ${NET_IFACE}
	${PREFIX}ip link del ${BR_TAP_IFACE}
	${PREFIX}ip link del ${TAP_IFACE}
}

start_encap_tx() {
	local ENCAP_TX_IFACE=$2
	local TX_IP_PORT=$3
	local RX_IP_PORT=$4
	local LOG=$5
	local BIN_ENCAP="$1/satencap"
	local PREFIX=""
	[ $# -eq 6 ] && PREFIX="ip netns exec $6 "

	${PREFIX}${BIN_ENCAP} ${OPT_ENCAP} -i ${ENCAP_TX_IFACE} -l ${TX_IP_PORT} -r ${RX_IP_PORT} &> ${LOG} &
	echo "$!"
}

start_encap_rx() {
	local ENCAP_RX_IFACE=$2
	local RX_IP_PORT=$3
	local TX_IP_PORT=$4
	local LOG=$5
	local BIN_DECAP="$1/satdecap"
	local PREFIX=""
	[ $# -eq 6 ] && PREFIX="ip netns exec $6 "

	${PREFIX}${BIN_DECAP} ${OPT_DECAP} -i ${ENCAP_RX_IFACE} -l ${RX_IP_PORT} -r ${TX_IP_PORT} &> ${LOG} &
	echo "$!"
}

check_running() {
	MESSAGE=$1
	shift
	PID_LIST=$*
	NEW_PID_LIST=""
	for PID in ${PID_LIST}; do
		LINES=$(ps -p ${PID} | wc -l)
		if [ ${LINES} -gt 1 ]; then
			NEW_PID_LIST="${PID} ${NEW_PID_LIST}"
		fi
	done
	if [ "${NEW_PID_LIST}_" != "_" ]; then
		echo "${MESSAGE}"
		echo "${NEW_PID_LIST}" >${PID_FILE}
	else
		rm ${PID_FILE}
	fi
}

stop() {
	PID_LIST=$(cat ${PID_FILE})
	for PID in ${PID_LIST}; do
		kill -2 ${PID}
	done
	sleep 1
	check_running "Warning: Some processes are still running. You can use the option \"force-stop\"." ${PID_LIST}
}

force_stop() {
	PID_LIST=$(cat ${PID_FILE})
	for PID in ${PID_LIST}; do
		kill -2 ${PID}
	done
	sleep 1
	for PID in ${PID_LIST}; do
		LINES=$(ps -p ${PID} | wc -l)
		if [ ${LINES} -gt 1 ]; then
			kill -15 ${PID}
		fi
	done
	sleep 1
	check_running "Error: Some processes are still running. Check the PIDs file to get processes PID: \"${PID_FILE}\"." ${PID_LIST}
}
