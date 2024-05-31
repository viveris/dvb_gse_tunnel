#!/bin/bash

# Copyright 2023, Viveris Technologies
# Distributed under the terms of the MIT License

ROOT=$(dirname $(readlink -e $0))
source ${ROOT}/testbed-common.sh

PID_FILE=$(readlink -e $0)
PID_FILE=$(dirname ${PID_FILE})/.pids.netns

usage() {
	echo "Usage: $(basename $0) COMMAND [OPTIONS]"
	echo "Handle the testbed in order to perform an UDP tunnel using encap/decap funtions"
	echo ""
	echo "    COMMAND    the command to perform on the testbed"
	echo "    OPTIONS    the options required by the command"
	echo ""
	echo "Possible commands and their options:"
	echo ""
	echo "    help"
	echo "        to display this message"
	echo ""
	echo "    setup"
	echo "        to setup the network configuration of the testbed"
	echo ""
	echo "    cleanup"
	echo "        to cleanup the network configuration of the testbed"
	echo ""
	echo "    start DIR"
	echo "        to start the encap/decap functions of the testbed"
	echo "        DIR is the directory path where binaries are stored"
	echo ""
	echo "    stop"
	echo "        to stop the encap/decap functions of the testbed"
	echo ""
	echo "    force-stop"
	echo "        to force the stop of  the encap/decap functions of the testbed"
}

setup() {
	# Create netns encap tx
	ip netns add encap_tx
	ip netns exec encap_tx sysctl net.ipv4.ip_forward=1 > /dev/null
	
	# Create netns encap rx
	ip netns add encap_rx
	ip netns exec encap_rx sysctl net.ipv4.ip_forward=1 > /dev/null
	
	# Connect the forward link
	ip link add name fwd_tx type veth peer name fwd_rx
	ip link set fwd_tx netns encap_tx
	ip link set fwd_rx netns encap_rx
	ip netns exec encap_tx ip address add 192.168.11.1/24 dev fwd_tx
	ip netns exec encap_tx ip link set dev fwd_tx up
	ip netns exec encap_rx ip address add 192.168.11.2/24 dev fwd_rx
	ip netns exec encap_rx ip link set dev fwd_rx up

	# Connect the return link
	ip link add name rtn_tx type veth peer name rtn_rx
	ip link set rtn_rx netns encap_tx
	ip link set rtn_tx netns encap_rx
	ip netns exec encap_tx ip address add 192.168.12.1/24 dev rtn_rx
	ip netns exec encap_tx ip link set dev rtn_rx address 00:00:00:00:00:ff
	ip netns exec encap_tx ip link set dev rtn_rx up
	ip netns exec encap_rx ip address add 192.168.12.2/24 dev rtn_tx
	ip netns exec encap_rx ip link set dev rtn_tx up

	# Setup encap rx
	#setup_encap_rx  TUN_IP        TUN_MAC           NET_IP   TGT_IP     GW_IFACE  GW_IP        GW_MAC            [NETNS]
	setup_encap_rx   192.168.101.1 00:00:00:00:00:01 10.0.0.1 10.0.0.254 rtn_tx    192.168.12.1 00:00:00:00:00:ff encap_rx

	# Setup encap tx
	#setup_encap_tx  TUN_IP        TUN_MAC           NET_IP     TGT_IP   GW_IP         GW_MAC            [NETNS]
	setup_encap_tx   192.168.101.2 00:00:00:00:00:02 10.0.0.254 10.0.0.1 192.168.101.1 00:00:00:00:00:01 encap_tx
}

cleanup() {
	# Cleanup encap tx
	cleanup_encap_tx encap_tx
	
	# Cleanup encap rx
	cleanup_encap_rx encap_rx

	# Disconnect the return link
	ip netns exec encap_rx ip link del dev rtn_tx
	
	# Disconnect the forward link
	ip netns exec encap_tx ip link del dev fwd_tx

	# Delete netns encap rx
	ip netns del encap_rx

	# Delete netns encap tx
	ip netns del encap_tx
}

start() {
	local BIN_DIR=$1

	local PID_LIST=""

	PID=$(start_encap_rx ${BIN_DIR} tap0 192.168.11.2:23000 192.168.11.1:23000 fwd_encap_rx.log encap_rx)
	PID_LIST="${PID_LIST} ${PID}"

	PID=$(start_encap_tx ${BIN_DIR} tap0 192.168.11.1:23000 192.168.11.2:23000 fwd_encap_tx.log encap_tx)
	PID_LIST="${PID_LIST} ${PID}"

	echo "${PID_LIST}" >${PID_FILE}
}

if [ $# -eq 0 ]; then
	echo "Invalid arguments"
	usage
	exit 1
fi
COMMAND=$1

case ${COMMAND} in
setup)
	if [ $# -ne 1 ]; then
		echo "Invalid arguments"
		usage
		exit 1
	fi
	setup
	;;

cleanup)
	if [ $# -ne 1 ]; then
		echo "Invalid arguments"
		usage
		exit 1
	fi
	cleanup
	;;

start)
	if [ $# -ne 2 ]; then
		echo "Invalid arguments"
		usage
		exit 1
	fi
	DIR=$(readlink -e $2)
	if [ ! -d "${DIR}" ]; then
		echo "Directory not found: \"${DIR}\""
		usage
		exit 2
	fi
	start ${DIR}
	;;

stop)
	if [ $# -ne 1 ]; then
		echo "Invalid arguments"
		usage
		exit 1
	fi
	stop
	;;

force-stop)
	if [ $# -ne 1 ]; then
		echo "Invalid arguments"
		usage
		exit 1
	fi
	force_stop
	;;

help)
	usage
	;;

*)
	echo "Invalid command \"${COMMAND}\""
	usage
	exit 1
	;;
esac
