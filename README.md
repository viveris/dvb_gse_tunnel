# dvb_gse_tunnel

A tunnel that connects two tap interfaces through udp using the gse protocol.

| Directory                             | Description                                                                                |
|---------------------------------------|--------------------------------------------------------------------------------------------|
| [c](c)                                | Source code written in C to perform a GSE tunnel                                           |
| [rust](rust)                          | Source code written in Rust to perform a GSE tunnel                                        |
| [rust_testimonial](rust_testimonial)  | Source code written in Rust to perform a transparent tunnel, to measure the TUNTAP impact  |


## Requirements

Before using this software, ensure you have the following prerequisites installed on your system:

- **Cargo and Rust**: Ensure you have Cargo, the Rust package manager, and the Rust compiler installed. You can install them by following the instructions on the official [Rust website](https://www.rust-lang.org/tools/install).

- **GCC**: Ensure you have GCC (GNU Compiler Collection) installed on your system. You can typically install GCC through your system's package manager. 

- **aclocal**: This software requires `aclocal` to be installed on your system. You can typically install `aclocal` as part of the GNU Autoconf package. On Debian-based systems, you can install it by running
  - ``` bash
    sudo apt-get install autoconf
    ```

- **libgse (language C)**: You also need the C libgse. Follow these instructions to install it.
  - First, you need to add the Net4Sat repository GPG Key:
    - ``` bash
      sudo mkdir /etc/apt/keyrings
      ```
    - ``` bash
      curl -sS https://raw.githubusercontent.com/CNES/net4sat-packages/master/gpg/net4sat.gpg.key | gpg --dearmor | sudo dd of=/etc/apt/keyrings/net4sat.gpg
      ```
  - Then, update sources.list with net4sat opensand repository
    - ``` bash
      cat << EOF | sudo tee /etc/apt/sources.list.d/github.net4sat.sources
      Types: deb
      URIs: https://raw.githubusercontent.com/CNES/net4sat-packages/master/jammy/
      Suites: jammy
      Components: stable
      Signed-By: /etc/apt/keyrings/net4sat.gpg
      EOF
      ```
  - Then you can install the libgse using :
    - ```
      sudo apt update
      ```
    - ```
      sudo apt install libgse-dev
      ```

- **iperf3**: `iperf3` is a tool for measuring the maximum TCP and UDP bandwidth performance. You can typically install `iperf3` through your system's package manager
  - ``` bash 
    sudo apt install iperf3
    ```
## Compile

### C

```bash
host$ cd c
host$ ./autogen.sh
host$ make
```

### Rust

```bash
host$ cd rust
host$ cargo build --release
```

## Testbed

The scripts to handle the testbed are stored in the [`testbed`](testbed) directory.

### Using network namespaces

This testbed uses 2 network namespaces on a host, connected by 2 veth pairs of interfaces:

  * one for the forward link (the encapsulated one);
  * one for the return link.

To set up this testbed:
```bash
host$ sudo ./testbed/testbed-netns.sh setup
```

To clean up this testbed:
```bash
host$ sudo ./testbed/testbed-netns.sh cleanup
```

To start the encapsulation binaries of this testbed:
```bash
host$ sudo ./testbed/testbed-netns.sh start BIN_DIR
```
where `BIN_DIR` can be:

  * c/src/tap_udp
  * rust/target/release
  * rust_testimonial/target/release
  * other...

To stop the encapsulation binaries of this testbed:
```bash
host$ sudo ./testbed/testbed-netns.sh stop
```

Once the testbed is set up and started, tests can be performed:

  * ping
```bash
host$ sudo ip netns exec encap_tx ping -I 10.0.0.254 -c 4 10.0.0.1
host$ sudo ip netns exec encap_rx ping -I 10.0.0.1 -c 4 10.0.0.254
```
  * iperf3
```bash
host$ sudo ip netns exec encap_rx iperf3 -B 10.0.0.1 -s
host$ sudo ip netns exec encap_tx iperf3 -B 10.0.0.254 -c 10.0.0.1 -ub 500M
```

### Using hosts

This testbed uses 2 hosts, connected on the same network.

To set up this testbed:
```bash
host1$ sudo ./testbed/testbed-host-encap.sh setup IFACE1
```
```bash
host2$ sudo ./testbed/testbed-host-decap.sh setup IFACE2
```
where `IFACE1` is the network interface of the host 1 and `IFACE2` is the one of the host 2 used to connect them together.

To clean up this testbed:
```bash
host1$ sudo ./testbed/testbed-host-encap.sh cleanup
```
```bash
host2$ sudo ./testbed/testbed-host-decap.sh cleanup
```

To start the encapsulation binaries of this testbed:
```bash
host1$ sudo ./testbed/testbed-host-encap.sh start BIN_DIR
```
```bash
host2$ sudo ./testbed/testbed-host-decap.sh start BIN_DIR
```
where `BIN_DIR` can be:

  * c/src/tap_udp
  * rust/target/release
  * rust_testimonial/target/release
  * other...

To stop the encapsulation binaries of this testbed:
```bash
host1$ sudo ./testbed/testbed-host-encap.sh stop
```
```bash
host2$ sudo ./testbed/testbed-host-decap.sh stop
```

Once the testbed is set up and started, tests can be performed:

  * ping
```bash
host1$ ping -I 10.0.0.254 -c 4 10.0.0.1
host2$ ping -I 10.0.0.1 -c 4 10.0.0.254
```
  * iperf3
```bash
host2$ iperf3 -B 10.0.0.1 -s
host1$ iperf3 -B 10.0.0.254 -c 10.0.0.1 -ub 500M
```
