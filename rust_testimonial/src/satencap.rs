// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

mod common;
use common::*;
use std::net::UdpSocket;

use std::process::exit;

struct SatEncap;
impl Binary for SatEncap {
    const DEFAULT_PAYLOAD_LENGTH: usize = 0;
    fn name() -> &'static str {
        "satencap"
    }

    fn usage() {
        eprintln!("Usage: {} -i TAP_IFACE -l LOCAL_ADDR_PORT -r REMOTE_ADDR_PORT
                [-p PAYLOAD_LEN]
                [-b BUFFER_LEN]
                [-t READ_TIMEOUT]
                [-h]
\n    Required arguments
        TAP_IFACE         the TAP interface which receives incoming IP packets
        LOCAL_ADDR_PORT   the address and port to use as source of UDP tunnel (format: \"ADDRESS:PORT\")
        REMOTE_ADDR_PORT  the address and port to use as destination of UDP tunnel (format: \"ADDRESS:PORT\")
\n    Optional arguments
        PAYLOAD_LEN       the constant size (bytes) of outcoming payload to the UDP tunnel. Set 0 to send packets of variable payload length (default: {})
        BUFFER_LEN        the max size (bytes) of incoming IP packet (default: {})
        READ_TIMEOUT      the timeout (ms) to read incoming IP packets (default: {})",
Self::name(),DEFAULT_PAYLOAD_LENGTH, DEFAULT_BUFFER_LENGTH, DEFAULT_READ_TIMEOUT
        );
        exit(1);
    }
}

fn runtime(a: Arguments) {
    let mut buffer = vec![0; GSE_MAX_PDU_LEN].into_boxed_slice();

    let socket = UdpSocket::bind(a.local).expect("couldn't bind to address");
    socket.connect(a.remote).expect("connect function failed");

    loop {
        let buffer_len = a.tap_iface.recv(&mut buffer[..a.buffer_len]).unwrap();
        let buffer = &buffer[..buffer_len];

        socket.send(&buffer).expect("couldn't send the packet");
    }
}

fn main() {
    let args = parse_arguments::<SatEncap>();
    runtime(args);
    println!("Hello, world!");
}
