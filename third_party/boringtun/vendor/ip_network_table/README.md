ip_network_table
========

IPv4 and IPv6 network fast lookup table.

[![Documentation](https://docs.rs/ip_network_table/badge.svg)](https://docs.rs/ip_network_table)
[![Build Status](https://travis-ci.com/JakubOnderka/ip_network_table.svg?branch=master)](https://travis-ci.com/JakubOnderka/ip_network_table)
[![Coverage Status](https://coveralls.io/repos/github/JakubOnderka/ip_network_table/badge.svg?branch=master)](https://coveralls.io/github/JakubOnderka/ip_network_table?branch=master)
[![Crates.io](https://img.shields.io/crates/v/ip_network_table.svg)](https://crates.io/crates/ip_network_table)

## Description

This crate provides storage and retrieval of IPv4 and IPv6 network prefixes.

Currently, it uses [`ip_network`](https://github.com/JakubOnderka/ip_network) crate, that provides IP network data structure and fork of
 [`treebitmap`](https://github.com/hroi/treebitmap) ([fork](https://github.com/JakubOnderka/treebitmap)) as backend, that provides fast lookup times, and a small memory footprint. Backend can be changed in future releases.

## Usage

Add this to your `Cargo.toml`:

```toml
[dependencies]
ip_network = "0.4"
ip_network_table = "0.2"
```

this to your crate root (not necessary when your project is Rust 2018 edition):

```rust
extern crate ip_network;
extern crate ip_network_table;
```

and then you can use it like this:

```rust
use std::net::{IpAddr, Ipv6Addr};
use ip_network::{IpNetwork, Ipv6Network};
use ip_network_table::IpNetworkTable;

let mut table = IpNetworkTable::new();
let network = IpNetwork::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
let ip_address = Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0x1);

assert_eq!(table.insert(network.clone(), "foo"), None);
// Get value for network from table
assert_eq!(table.longest_match(ip_address), Some((network, &"foo")));
```

Minimal required version of Rust compiler is 1.31 (because of `ip_network` crate). 
