#!/bin/bash

# Copyright 2023, Viveris Technologies
# Distributed under the terms of the MIT License

ROOT=$(dirname $(readlink -e $0))
source ${ROOT}/testbed-common.sh

PID_FILE=$(readlink -e $0)
PID_FILE=$(dirname ${PID_FILE})/.pids.host-decap

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
	echo "    setup IFACE"
	echo "        to setup the network configuration of the testbed"
	echo "        IFACE is the network interface to use for the UDP tunnel"
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
	local IFACE=$1
	
	# Create vlan tx
	ip link add link ${IFACE} name rtn_tx type vlan id 2
	ip addr add 192.168.12.2/24 dev rtn_tx
	ip link set dev rtn_tx up

	# Create vlan rx
	ip link add link ${IFACE} name fwd_rx type vlan id 1
	ip addr add 192.168.11.2/24 dev fwd_rx
	ip link set dev fwd_rx up
	
	# Setup encap rx
	#setup_encap_rx  TUN_IP        TUN_MAC           NET_IP   TGT_IP     GW_IFACE  GW_IP        GW_MAC
	setup_encap_rx   192.168.101.1 00:00:00:00:00:01 10.0.0.1 10.0.0.254 rtn_tx    192.168.12.1 00:00:00:00:00:ff
}

cleanup() {
	# Cleanup encap rx
	cleanup_encap_rx

	# # Delete the return link rx iface
	ip link del dev rtn_tx
	
	# # Delete the forward link tx iface
	ip link del dev fwd_rx
}

start() {
	local BIN_DIR=$1

	local PID_LIST=""

	PID=$(start_encap_rx ${BIN_DIR} tap0 192.168.11.2:23000 192.168.11.1:23000 fwd_encap_rx.log)
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
	if [ $# -ne 2 ]; then
		echo "Invalid arguments"
		usage
		exit 1
	fi
	IFACE=$2
	ip link show dev ${IFACE} &> /dev/null
	if [ $? -ne 0 ]; then
		echo "Interface not found: \"${IFACE}\""
		usage
		exit 2
	fi
	setup ${IFACE}
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
