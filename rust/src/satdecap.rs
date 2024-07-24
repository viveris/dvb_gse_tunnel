// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

mod common;
use dvb_gse_rust::{
    crc::DefaultCrc,
    header_extension::SimpleMandatoryExtensionHeaderManager,
    gse_decap::{Decapsulator, DecapStatus, GseDecapMemory, SimpleGseMemory},
};
use std::{net::UdpSocket, process::exit};

use common::*;

struct SatDecap;
impl Binary for SatDecap {
    const DEFAULT_PAYLOAD_LENGTH: usize = 2048;
    fn name() -> &'static str {
        "satdecap"
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
        PAYLOAD_LEN       the constant size (bytes) of incoming payload to the UDP tunnel (default: {})
        BUFFER_LEN        the max size (bytes) of outcoming IP packet (default: {})
        READ_TIMEOUT      the timeout (ms) to read incoming payload from UDP tunnel (default: {})",
Self::name(), DEFAULT_PAYLOAD_LENGTH, DEFAULT_BUFFER_LENGTH, DEFAULT_READ_TIMEOUT
        );
        exit(1);
    }
}

async fn runtime(a: Arguments) {
    let socket = UdpSocket::bind(a.local).expect("couldn't bind to address");
    socket.connect(a.remote).expect("connect function failed");

    let buffer0 = vec![0; GSE_MAX_PDU_LEN].into_boxed_slice();
    let buffer1 = vec![0; GSE_MAX_PDU_LEN].into_boxed_slice();
    let buffer2 = vec![0; GSE_MAX_PDU_LEN].into_boxed_slice();
    let mut payload = vec![0; GSE_MAX_PACKET_LENGTH].into_boxed_slice();

    let mut gse_decap_memory = SimpleGseMemory::new(1, GSE_MAX_PACKET_LENGTH, 0, 0);

    gse_decap_memory.provision_storage(buffer0).unwrap();
    gse_decap_memory.provision_storage(buffer1).unwrap();
    gse_decap_memory.provision_storage(buffer2).unwrap();
    
    let mut decapsulator = Decapsulator::new(gse_decap_memory, DefaultCrc {}, SimpleMandatoryExtensionHeaderManager {});
    loop {
        let payload_len = socket.recv(&mut payload).unwrap();
        let payload = &payload[..payload_len];


            let status = decapsulator.decap(payload);

            let (buffer, buffer_len) = match status {
                Ok((DecapStatus::CompletedPkt(buffer, metadata), _)) => (buffer, metadata.pdu_len),
                Ok((DecapStatus::FragmentedPkt(_), _)) => break,
                _ => {
                    eprintln!("ERROR when decap {:?}", status);
                    break;
                }
            };

            a.tap_iface.send(&buffer[..buffer_len]).await.unwrap();
            decapsulator.provision_storage(buffer).unwrap();
    
    }
}

#[tokio::main]
async fn main() {
    let args = parse_arguments::<SatDecap>();
    runtime(args).await;
}
