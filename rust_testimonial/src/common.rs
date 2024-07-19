// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

use std::{
    env,
    net::{SocketAddr, ToSocketAddrs},
    time::Duration,
};

use tokio_tun::{Tun, TunBuilder};

pub const DEFAULT_PAYLOAD_LENGTH: usize = 2048;
pub const DEFAULT_BUFFER_LENGTH: usize = 8192; // bytes
pub const DEFAULT_READ_TIMEOUT: u64 = 100; // ms
pub const GSE_MAX_PDU_LEN: usize = 65535;

pub trait Binary {
    const DEFAULT_PAYLOAD_LENGTH: usize;
    fn name() -> &'static str;
    fn usage();
}

#[allow(dead_code)]
pub struct Arguments {
    pub tap_iface: Tun,
    pub local: SocketAddr,
    pub remote: SocketAddr,
    pub payload_len: usize,
    pub buffer_len: usize,
    pub _read_timeout: Duration,
}

pub fn parse_arguments<B: Binary>() -> Arguments {
    let args = env::args();
    let mut iter_args = args.into_iter();

    let mut interface: Option<String> = None;
    let mut local_udp: Option<String> = None;
    let mut remote_udp: Option<String> = None;
    let mut payload_len: usize = B::DEFAULT_PAYLOAD_LENGTH;
    let mut buffer_len: usize = DEFAULT_BUFFER_LENGTH;
    let mut read_timeout: u64 = DEFAULT_READ_TIMEOUT;

    iter_args.next().unwrap();
    while let Some(arg) = iter_args.next() {
        match arg.as_str() {
            "-h" => B::usage(),
            "-i" => {
                interface = Some(
                    iter_args
                        .next()
                        .expect("-i must be followed by an interface name"),
                );
            }
            "-l" => {
                local_udp = Some(
                    iter_args
                        .next()
                        .expect("-l must be followed by ADDRESS:PORT"),
                );
            }
            "-r" => {
                remote_udp = Some(
                    iter_args
                        .next()
                        .expect("-l must be followed by ADDRESS:PORT"),
                );
            }
            "-p" => {
                payload_len = iter_args
                    .next()
                    .expect("-p must be followed by a usize in bytes")
                    .parse()
                    .expect("Invalid payload length: the value must be an usize in bytes");
            }
            "-b" => {
                buffer_len = iter_args
                    .next()
                    .expect("-b must be followed by a usize in bytes")
                    .parse()
                    .expect("Invalid buffer length: the value must be an usize in bytes");
            }
            "-t" => {
                read_timeout = iter_args
                    .next()
                    .expect("-b must be followed by a usize in ms")
                    .parse()
                    .expect("Invalid reading timeout: the value must be an usize in ms");
            }

            _ => {
                panic!("unknown argument {}", arg);
            }
        }
    }

    let local = local_udp
        .expect("the local address must be provided")
        .to_socket_addrs()
        .expect("the local address is wrong")
        .next()
        .unwrap();

    let remote = remote_udp
        .expect("the remote address must be provided")
        .to_socket_addrs()
        .expect("the remote adresse is wrong")
        .next()
        .unwrap();

    let config = TunBuilder::new()
        .name(
            interface
                .expect("the tap interface must be provided")
                .as_str(),
        )
        .tap(true)
        .packet_info(false);

    let tap_iface = config.try_build().expect("Failed to open TUN interface");

    let read_timeout = Duration::from_millis(read_timeout);

    Arguments {
        tap_iface,
        local,
        remote,
        payload_len,
        buffer_len,
        _read_timeout: read_timeout,
    }
}
