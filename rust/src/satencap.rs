// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

mod common;
use common::*;
use std::net::UdpSocket;

use dvb_gse_rust::{
    crc::DefaultCrc,
    gse_encap,
    label::{Label, LabelType},
};
use std::process::exit;

const FRAG_ID: u8 = 0;
const PROTOCOL: u16 = 9029;

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
    let mut payload = vec![0; GSE_MAX_PACKET_LENGTH].into_boxed_slice();
    let desired_len: usize = if a.payload_len == 0 {
        payload.len()
    } else {
        a.payload_len
    };

    let mut encapsulator = gse_encap::Encapsulator::new(DefaultCrc {});

    let socket = UdpSocket::bind(a.local).expect("couldn't bind to address");
    socket.connect(a.remote).expect("connect function failed");

    let mut counter: u64 = 1;
    loop {
        let buffer_len = a.tap_iface.recv(&mut buffer[..a.buffer_len]).unwrap();
        let buffer = &buffer[..buffer_len];

        // Create a different label for each pdu
        let label: [u8; 6] = [
            ((counter >> 56) & 0xff) as u8,
            ((counter >> 48) & 0xff) as u8,
            ((counter >> 32) & 0xff) as u8,
            ((counter >> 16) & 0xff) as u8,
            ((counter >> 8) & 0xff) as u8,
            (counter & 0xff) as u8,
        ];

        let mut context: Option<gse_encap::ContextFrag> = None;
        loop {
            counter += 1;

            let metadata = gse_encap::EncapMetadata::new(
                PROTOCOL,
                Label::new(&LabelType::SixBytesLabel, &label),
            );

            let status = match context {
                Some(ctx) => encapsulator.encap_frag(buffer, &ctx, &mut payload),
                None => encapsulator.encap(buffer, FRAG_ID, metadata, &mut payload),
            };

            let mut payload_len = match status.unwrap() {
                gse_encap::EncapStatus::FragmentedPkt(len, ctx) => {
                    context = Some(ctx);
                    len as usize
                }
                gse_encap::EncapStatus::CompletedPkt(len) => {
                    context = None;
                    len as usize
                }
            };

            if payload_len < desired_len && a.payload_len != 0 {
                payload[payload_len..].fill(0);
                payload_len = desired_len;
            }

            socket
                .send(&payload[..payload_len])
                .expect("couldn't send the packet");

            if context == None {
                break;
            }
        }
    }
}

fn main() {
    let args = parse_arguments::<SatEncap>();
    runtime(args);
    println!("Hello, world!");
}
