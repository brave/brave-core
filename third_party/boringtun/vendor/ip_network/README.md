ip_network
========

IPv4 and IPv6 network structs.

[![Documentation](https://docs.rs/ip_network/badge.svg)](https://docs.rs/ip_network)
[![Build Status](https://travis-ci.com/JakubOnderka/ip_network.svg?branch=master)](https://travis-ci.com/JakubOnderka/ip_network)
[![Coverage Status](https://coveralls.io/repos/github/JakubOnderka/ip_network/badge.svg?branch=master)](https://coveralls.io/github/JakubOnderka/ip_network?branch=master)
[![Crates.io](https://img.shields.io/crates/v/ip_network.svg)](https://crates.io/crates/ip_network)

## Usage

Add this to your `Cargo.toml`:

```toml
[dependencies]
ip_network = "0.4"
```

this to your crate root (not necessary when your project is Rust 2018 edition):

```rust
extern crate ip_network;
```

and then you can use it like this:

```rust
use std::net::Ipv4Addr;
use ip_network::Ipv4Network;

let ip_network = Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
assert_eq!(Ipv4Addr::new(192, 168, 1, 0), ip_network.network_address());
assert_eq!(24, ip_network.netmask());
assert_eq!(254, ip_network.hosts().len());
assert_eq!("192.168.1.0/24", ip_network.to_string());
```

Minimal required version of Rust compiler is:
- 1.31 for version 0.3 and newer (because of 2018 edition),
- 1.26 for version 0.2 (because of support u128 data type),
- for older compiler you can use 0.1 version.   

## Optional features

When using this crate, you can choose to compile with these features:

### Serde support

To enable serialization and deserialization by [Serde framework](https://serde.rs), 
just add `serde` feature to package in your `Cargo.toml`:

```toml
[dependencies]
ip_network = { version = "0.4", features = ["serde"] }
``` 

### Diesel support

To enable support for [diesel](https://diesel.rs) [CIDR type] for PostgreSQL, 
just add `diesel` feature to package in your `Cargo.toml`:

```toml
[dependencies]
ip_network = { version = "0.4", features = ["diesel"] }
``` 

You can then use `ip_network::diesel_support::PqCidrExtensionMethods` trait for CIDR operators support.


### Postgres support

To enable support for [postgres](https://github.com/sfackler/rust-postgres) crate [CIDR type], 
just add `postgres` feature to package in your `Cargo.toml`:

```toml
[dependencies]
ip_network = { version = "0.4", features = ["postgres"] }
``` 

## Comparison with `ipnetwork` crate

Similar functionality also provides [ipnetwork](https://github.com/achanda/ipnetwork) crate. 
This table shows the differences between these two crates:


| Feature              | ip_network | ipnetwork |
|----------------------|------------|-----------|
| IPv4                 |      ✓     |     ✓     |
| IPv6                 |      ✓     |     ✓     |
| IPv4 and IPv6 enum   |      ✓     |     ✓     |
| IPv4 network types   |      ✓     |           |
| IPv6 network types   |      ✓     |           |
| Hosts iterator       |      ✓     |     ✓     | 
| Subnetworks iterator |      ✓     |           |
| Check host bits set  |      ✓     |           |
| Serde                |      ✓     |     ✓     |
| Serde binary         |      ✓     |           |
| Diesel CIDR          |      ✓     |     ✓     |
| Diesel operators     |      ✓     |           |
| Postgres CIDR        |      ✓     |           |
| IPv4 string parsing  | 65 ns      | 379 ns    |
| IPv6 string parsing  | 126 ns     | 434 ns    |
| IPv4 contains method | 7 ns       | 15 ns     |
| IPv6 contains method | 28 ns      | 49 ns     |

[CIDR type]: https://www.postgresql.org/docs/current/datatype-net-types.html#DATATYPE-CIDR
